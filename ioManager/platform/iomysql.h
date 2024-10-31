class mysql_client {
    //global things
    inline static std::atomic_flag global_init = ATOMIC_FLAG_INIT;
    inline static std::atomic<int> mysql_count = 0;
#if IO_USE_SELECT
    inline static std::map<uint64_t, io::coPromise<>> select_rbt;
    inline static std::atomic_flag select_rbt_busy = ATOMIC_FLAG_INIT;              //tcp client (connect oriented)
    inline static void selectDrive()
    {
        static fd_set read_fds;
        FD_ZERO(&read_fds);
        uint64_t fd_max = 0;
        while (select_rbt_busy.test_and_set(std::memory_order_acquire));
        if (select_rbt.size())
        {
            for (auto &[key, promise] : select_rbt)
            {
                if (promise.completable() == false)
                    continue;
                FD_SET(key, &read_fds);
            }
            fd_max = select_rbt.rbegin()->first + 1;
        }
        select_rbt_busy.clear();

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100; // fixed 100us per circle
        int result = select(fd_max, &read_fds, nullptr, nullptr, &timeout);
        if (result > 0)
        {
            while (select_rbt_busy.test_and_set(std::memory_order_acquire));
            for (auto &[key, promise] : select_rbt)
            {
                if (FD_ISSET(key, &read_fds))
                {
                    promise.complete();
                }
            }
            select_rbt_busy.clear();
        }
    }
#endif
    inline static void go()
    {
        while (global_init.test_and_set(std::memory_order_acquire))
        {
            if (mysql_count.load() > 0)
            {
                // select
                selectDrive();
            }
            else
            {
                mysql_count.wait(0, std::memory_order_acquire);
            }
        }
    }
    inline static void global_startup()
    {
        if (global_init.test_and_set() == false)
        {
            std::thread(mysql_client::go).detach();
        }
    }
    inline static void global_cleanup()
    {
	    global_init.clear();
    }


public:
    uint64_t sock;
    MYSQL handle;
    io::coPromise<> awaiter;    //only complete, never returns aborted.
public:
    //stmt, bugly.
    struct STMT{
        mysql_client* _mysql;
        MYSQL_STMT* _h;
        inline STMT(mysql_client* mysql)
        {
            _mysql = mysql;
            _h = mysql_stmt_init(&mysql->handle);
        }
        inline ~STMT()
        {
            mysql_stmt_close(_h);
        }
        inline io::coTask prepare(io::coPromise<> &promise, const char *query, size_t length, std::chrono::microseconds overtime) // prepare after connect!
        {
            _mysql->awaiter.setTimeout(overtime);
            promise.setTimeout(overtime);

            int ret, status = mysql_stmt_prepare_start(&ret, _h, query, length);
            task_await(_mysql->awaiter);

            if (_mysql->awaiter.isCompleted())
            {
                status = mysql_stmt_prepare_cont(&ret, _h, status);
                if (ret)
                    promise.complete();
                else
                    promise.abort();
            }
        }
        inline operator MYSQL_STMT *() { return _h; }
        STMT(const STMT &) = delete;
        void operator=(STMT&) = delete;
    };
    inline mysql_client(io::ioManager *m)
    {
        global_startup();
        awaiter = io::coPromise<>(m);
        mysql_init(&handle);
        mysql_options(&handle, MYSQL_OPT_NONBLOCK, 0);      //if blocking, this library is meaningless.
        mysql_count++;
    }
    inline ~mysql_client()
    {
        this->close();
        mysql_count--;
    }
    inline io::coTask connect(
                        io::coPromise<> &promise,
                        const char *host,
                        const char *user,
                        const char *passwd,
                        const char *db,
                        unsigned int port,
                        std::chrono::microseconds overtime)
    {
        MYSQL *ret;
        int status = mysql_real_connect_start(&ret, &handle, host, user, passwd, db, port, NULL, 0);
        sock = (uint64_t)mysql_get_socket(&handle);

        while (select_rbt_busy.test_and_set(std::memory_order_acquire));
        select_rbt.insert(std::pair<uint64_t, io::coPromise<>>(sock, awaiter));
        select_rbt_busy.clear(std::memory_order_release);

        awaiter.setTimeout(overtime);
        promise.setTimeout(overtime);
        task_await(awaiter);

        if(awaiter.isCompleted())
        {
            status = mysql_real_connect_cont(&ret, &handle, status);
            if (ret)
                promise.complete();
            else
                promise.abort();
        }
    }
    inline io::coTask query(io::coPromise<> &promise, const char* q, size_t size, std::chrono::microseconds overtime)
    {
        awaiter.setTimeout(overtime);
        promise.setTimeout(overtime);

        int ret, status = mysql_real_query_start(&ret, &handle, q, size);
        task_await(awaiter);

        if (awaiter.isCompleted())
        {
            status = mysql_real_query_cont(&ret, &handle, status);
            if (ret)
                promise.complete();
            else
                promise.abort();
        }
    }
    inline io::coTask store_result(io::coPromise<> &promise, MYSQL_RES **ret, std::chrono::microseconds overtime)
    {
        int status = mysql_store_result_start(ret, &handle);
        if (*ret)
        {
            promise.complete();
        }
        else
        {
            awaiter.setTimeout(overtime);
            promise.setTimeout(overtime);
            task_await(awaiter);
            if (awaiter.isCompleted())
            {
                status = mysql_store_result_cont(ret, &handle, status);
                if (*ret)
                    promise.complete();
                else
                    promise.abort();
            }
        }
    }
    inline io::coTask execute(io::coPromise<> &promise, const STMT& stmt, std::chrono::microseconds overtime)
    {
        awaiter.setTimeout(overtime);
        promise.setTimeout(overtime);

        int ret, status = mysql_stmt_execute_start(&ret, stmt._h);
        task_await(awaiter);

        if (awaiter.isCompleted())
        {
            status = mysql_stmt_execute_cont(&ret, stmt._h, status);
            if (ret)
                promise.complete();
            else
                promise.abort();
        }
    }
    inline void close()
    {
        while (select_rbt_busy.test_and_set(std::memory_order_acquire));
        select_rbt.erase(sock);
        select_rbt_busy.clear(std::memory_order_release);

        mysql_close(&handle);
    }
    mysql_client(const mysql_client &) = delete;
    void operator=(mysql_client&) = delete;
    inline std::string escape_string(const std::string &input)
    {
        size_t input_length = input.length();
        std::string escaped_string(input_length * 2, '\0');

        size_t escaped_length = mysql_real_escape_string(&handle, &escaped_string[0], input.c_str(), input_length);
        escaped_string.resize(escaped_length);

        return escaped_string;
    }
};