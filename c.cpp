#include<stdio.h>
//#include<mariadb/mysql.h>
#define IO_USE_SELECT 1
#include "memManager/demo.h"
#include "ioManager/ioManager.h"

io::coTask jointest(io::coPara para)
{
    io::coPromise<> prom(para.mngr);
    prom.setTimeout(std::chrono::milliseconds(1000));
    task_await(prom);
    co_return;
}

io::coTask benchmark_coroutine(io::coPara para)
{
    while (1)
    {
        io::coTask joint = jointest(para);
        task_join(joint);
        static int i = 0;
        //std::cout << i++ << std::endl;    //watch cpu usage rate curve. it will be tidal.
    }
}

io::coTask test_tcp_client(io::coPara para) {
    io::tcp_client_socket socket;
    io::coPromise<io::socketData> fu(para.mngr);

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(54321);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    static std::atomic<int> numSum = 0;
    int num = numSum.fetch_add(1);
    std::ostringstream oss;
    oss << num;
    std::string numStr = oss.str();

    while (1)
    {
        while (socket.open() == io::err::failed);
        socket.connect(fu, serverAddr);
        fu.setTimeout(std::chrono::seconds(5));

        io::socketData* data = task_await(fu);

        if (fu.isCompleted())
        {
            socket.send(numStr.c_str(), numStr.length());
            std::cout << num << "TCP client established and sent." << data->data << std::endl;
            fu.reset();
            fu.setTimeout(std::chrono::milliseconds(5000));
            data = task_await(fu);
            if (fu.isAborted())
            {
                std::cout << num << "TCP client was closed. " << std::endl;
            }
        }
        else if (fu.isAborted())
        {
            std::cout << num << "TCP client failed. " << std::endl;
        }
        else
        {
            std::cout << num << "TCP client : try connect timeout" << std::endl;
        }
        fu.reset();
    }
}

io::coTask test_tcp_server_connect(io::coPara para, io::tcp_client_socket newSock) {
    io::coPromise<io::socketData> data_prom = newSock.findPromise();
    std::cout << "TCP server : connect inbound. " << std::endl;
    while (1)
    {
        data_prom.setTimeout(std::chrono::seconds(10));
        io::socketData* pdata = task_await(data_prom);
        if (data_prom.isTimeout())
        {
            std::cout << "TCP server : timeout, server closed it. " << std::endl;
        }
        else if (data_prom.isCompleted())
        {
            if (pdata->depleted == 0)
            {
                data_prom.reset();
                continue;
            }
            std::cout << "TCP server : receive: " << pdata->data << " then close." << std::endl;
        }
        else
        {
            std::cout << "TCP server : closed. " << std::endl;
        }
        co_return;
    }
}

io::coTask test_tcp_server(io::coPara para) {
    io::coPromise<io::tcp_client_socket> client_prom = (para.mngr);

    io::tcp_server_socket server;
    while (server.open() == io::err::failed);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(54321);

    if(server.bind(client_prom, server_addr) == io::err::failed)
        std::cerr << "Port is already in use.\n";
    else
        while (1)
        {
            io::tcp_client_socket* sock = task_await(client_prom);
            test_tcp_server_connect(para, std::move(*client_prom.getPointer()));
            client_prom.reset();
        }
}

io::coTask test_udp_client(io::coPara para) {
    io::udp_socket socket;
    io::coPromise<io::socketDataUdp> fu(para.mngr);

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12356);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    static std::atomic<int> numSum = 0;
    int num = numSum.fetch_add(1);
    std::ostringstream oss;
    oss << num;
    std::string numStr = oss.str();

    while (socket.open() == io::err::failed);
    while (1)
    {
        socket.sendtoAndBind(numStr.c_str(),numStr.length(), serverAddr, fu);
        std::cout << num << "UDP client sentto. " << std::endl;
        fu.setTimeout(std::chrono::seconds(5));

        io::socketData* data = task_await(fu);

        if (fu.isCompleted())
        {
            std::cout << num << "UDP client callback recv:" << data->data << std::endl;
            data->depleted = 0;
        }
        else if (fu.isAborted())
        {
            std::cout << num << "UDP client failed. " << std::endl;
        }
        else
        {
            std::cout << num << "UDP client : try connect timeout" << std::endl;
        }
        fu.reset();
    }
}

io::coTask test_udp_server(io::coPara para) {
    io::udp_socket socket;
    io::coPromise<io::socketDataUdp> fu(para.mngr);

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12356);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while (socket.open() == io::err::failed);
    socket.bind(fu, serverAddr);
    std::string cbstr = "call back string.";
    while (1)
    {
        io::socketDataUdp* data = task_await(fu);
        std::cout << "UDP server recv:" << data->data << std::endl;
        data->depleted = 0;
        socket.sendto(cbstr.c_str(), cbstr.size(), data->addr);
        fu.reset();
    }
}

io::coTask testping(io::coPara para)
{
    io::icmp_client_socket socket;
    io::coPromise<io::socketDataUdp> fu(para.mngr);

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    //inet_pton(AF_INET, "114.514.19.19", &serverAddr.sin_addr);    //invalid ip
    //inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);        //local host
    inet_pton(AF_INET, "183.2.172.42", &serverAddr.sin_addr);       //baidu.com

    while (1)
    {
        socket.sendPing(fu, serverAddr, std::chrono::seconds(1));

        io::socketData* data = task_await(fu);

        if (fu.isCompleted())
        {
            std::cout << "ICMP received bytes: " << data->depleted << " delay: " << socket.getDelay().count() / 1000000 << " ms" << std::endl;
            data->depleted = 0;
        }
        else if (fu.isAborted())
        {
            std::cout << "ICMP fail connect" << std::endl;
        }
        else
        {
            std::cout << "ICMP timeout" << std::endl;
        }

        fu.reset();
        fu.setTimeout(std::chrono::milliseconds(500));  //wait 500ms
        task_await(fu);
        fu.reset();
    }
}

std::string sockaddrInToString(const sockaddr_in& addr) {
    char ipStr[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr.sin_addr, ipStr, sizeof(ipStr)) == nullptr) {
        perror("inet_ntop");
        return "";
    }

    uint16_t port = ntohs(addr.sin_port);

    return std::string(ipStr) + ":" + std::to_string(port);
}

io::coTask test_dns_client(io::coPara para) {
    io::dns_client dns(para);
    io::coPromise<sockaddr_in> prom(para.mngr);

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(53);
    serverAddr.sin_addr.s_addr = inet_addr("8.8.8.8");
    std::string domain = "www.google.com";
    sockaddr_in addr_domain;

    while (dns.socket.open() == io::err::failed);
    while (1)
    {
        auto task = dns.query(prom, domain, serverAddr, std::chrono::seconds(5));
        std::cout << "DNS client sentto. " << std::endl;
        sockaddr_in* paddr = task_await(prom);

        if (prom.isCompleted())
        {
            std::cout << "DNS client callback recv:" << sockaddrInToString(*paddr) << std::endl;
        }
        else if (prom.isAborted())
        {
            std::cout << "DNS client failed. " << std::endl;
        }
        else
        {
            std::cout << "DNS client : try connect timeout" << std::endl;
        }
        prom.reset();
    }
}

io::coTask test_http_client(io::coPara para)
{
    io::httpRequest request;
    request.method = "GET";
    request.url = "/";
    request.httpVersion = "HTTP/1.1";

    request.addHeader("Host", "www.taobao.com");
    request.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/87.0.4280.88 Safari/537.36");
    request.addHeader("Accept", "text/plain,*/*;q=0.8");
    //request.addHeader("Accept-Encoding", "gzip, deflate, br");
    request.addHeader("Accept-Language", "en-US,en;q=0.9");
    request.addHeader("Connection", "keep-alive");

    while (1)
    {
        io::http_client httpc(para);
        io::coPromise<io::httpResponce> respon = httpc.send(para.mngr, &httpc, request, "www.taobao.com");

        respon.setTimeout(std::chrono::seconds(7));
        io::httpResponce* resp = task_await(respon);

        if (respon.isAborted())
        {
            std::cout << "http aborted" << std::endl;
        }
        else if (respon.isTimeout())
        {
            std::cout << "http timeout" << std::endl;
        }
        else
        {
            std::cout << "rec: " << resp->toString() << std::endl;
        }
    }
}

io::coTask test_https_client_proxy(io::coPara para)
{
    sockaddr_in proxyAddr;
    memset(&proxyAddr, 0, sizeof(proxyAddr));
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_port = htons(9998);                   //nginx proxy http to https in specified localhost port
    proxyAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    io::httpRequest request;
    request.method = "GET";
    request.url = "/";
    request.httpVersion = "HTTP/1.1";

    request.addHeader("Host", "www.csdn.net");
    request.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/87.0.4280.88 Safari/537.36");
    request.addHeader("Accept", "text/plain,*/*;q=0.8");
    //request.addHeader("Accept-Encoding", "gzip, deflate, br");
    request.addHeader("Accept-Language", "en-US,en;q=0.9");
    request.addHeader("Connection", "keep-alive");

    while (1)
    {
        io::http_client httpc(para);
        io::coPromise<io::httpResponce> respon = httpc.send(para.mngr, &httpc, request, proxyAddr);

        respon.setTimeout(std::chrono::seconds(7));
        io::httpResponce* resp = task_await(respon);

        if (respon.isAborted())
        {
            std::cout << "http aborted" << std::endl;
        }
        else if (respon.isTimeout())
        {
            std::cout << "http timeout" << std::endl;
        }
        else
        {
            std::cout << "rec: " << resp->toString() << std::endl;
        }
    }
}

io::coTask test_http_server_connect(io::coPara para, io::tcp_client_socket newSock) {
    io::httpRequest http_requ;
    io::coPromise<io::socketData> data_prom = newSock.findPromise();
    //std::cout << "HTTP server : connect inbound. " << std::endl;
    while (1)
    {
        data_prom.setTimeout(std::chrono::seconds(10));
        io::socketData* pdata = task_await(data_prom);
        if (data_prom.isTimeout())
        {
            std::cout << "HTTP server : timeout, server closed it. " << std::endl;
        }
        else if (data_prom.isCompleted())
        {
            if (pdata->depleted == 0)
            {
                data_prom.reset();
                continue;
            }
            else
            {
                auto fromcharret = http_requ.fromChar(pdata->data, pdata->depleted);
                pdata->depleted = 0;
                if (fromcharret == io::err::ok)
                {
                    //std::cout << "HTTP server : receive: " << http_requ.toString() << " then close." << std::endl;
                    static std::atomic<int> count = 0;
                    //std::cout << "HTTP server count: " << count.fetch_add(1) << std::endl; 
                    io::httpResponce resp;
                    resp.statusCode = 410;
                    resp.reasonPhrase = "server unavailable.";
                    resp.httpVersion = "HTTP/1.1";
                    auto resp_str = resp.toString();
                    newSock.send(resp_str.c_str(), resp_str.size());
                    co_return;
                }
                else if(fromcharret == io::err::less)
                {
                    data_prom.reset();
                    continue;
                }
                else
                    co_return;      //sock will close in destructor
            }
            
        }
        else
        {
            std::cout << "HTTP server : closed. " << std::endl;
        }
        co_return;
    }
}

io::coTask test_https_server_proxy(io::coPara para) {
    io::coPromise<io::tcp_client_socket> client_prom = (para.mngr);

    io::tcp_server_socket server;
    while (server.open() == io::err::failed);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9999);

    if (server.bind(client_prom, server_addr) == io::err::failed)
        std::cerr << "Port is already in use.\n";
    else
        while (1)
        {
            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
            io::tcp_client_socket* sock = task_await(client_prom);
            test_http_server_connect(para, std::move(*client_prom.getPointer()));
            client_prom.reset();
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            std::chrono::duration duration = end - start;
            std::cout << duration << " spent" << std::endl;
        }
    co_return;
}

#ifdef _mysql_h
io::coTask test_mysql(io::coPara para){
    io::coPromise<> prom(para.mngr);

    io::mysql_client my_c(para.mngr);
    my_c.connect(prom, "127.0.0.1", "starwars", "1", "starwars", 3306, std::chrono::seconds(2));
    task_await(prom);
    if (prom.isCompleted())
    {
        std::cout << "mysql login successful" << std::endl;
    }
    else if (prom.isAborted())
    {
        std::cout << "mysql login failed" << std::endl;
        co_return;
    }
    else
    {
        std::cout << "mysql timeout" << std::endl;
        co_return;
    }
    while (1)
    {
        const char instr[] = "SELECT * FROM user";

        prom.reset();
        my_c.query(prom, instr, sizeof(instr), std::chrono::seconds(2));
        task_await(prom);

        MYSQL_RES *result;
        prom.reset();
        my_c.store_result(prom, &result, std::chrono::seconds(2));
        task_await(prom);

        MYSQL_ROW row = mysql_fetch_row(result);
        
        if (row)
        {
            static int i = 0;
            std::cout << "mysql fetch successful:" << i++ << std::endl;
            std::cout << "uid: " << row[0] << ", qq number: " << row[1] << ", nickname: " << row[2] << "\n";
        }
        mysql_free_result(result);
    }
}
#endif

int main()
{
    //coroutine benchmark
    if (false)
    {
        io::ioManager::auto_go(1);  //single thread
        for (int i = 0; i < 1000000; i++)   //less than 1 us per task recircle when in 1M coroutines (condition: same timeout for each promise)
        {
            io::ioManager::auto_once(benchmark_coroutine);
        }
        std::this_thread::sleep_for(std::chrono::years(30));
    }

    //montgomery test
    if (false)
    {
        io::encrypt::rsa::BigInt a = 453437783;
        io::encrypt::rsa::BigInt b = 867843654;
        io::encrypt::rsa::BigInt n = 437547537;

        a = a << (uint32_t)100;
        b = b << (uint32_t)100;

        a = a * (uint32_t)143454543;
        b = b * (uint32_t)154788792;
        n = n * (uint32_t)161554437;

        a = a * (uint32_t)378634322;
        b = b * (uint32_t)486731322;
        for (int i = 0; i < 5; i++)
            n = n * (uint32_t)135637845;

        //n = n + (uint32_t)1;

        io::encrypt::rsa::mon_domain md = n;
        md.respawn();
        io::encrypt::rsa::BigInt res;
        io::encrypt::rsa::BigInt res2;
        for (int i = 0; i < 10000; i++)
        {
            res = (a * b) % n;
            res2 = io::encrypt::rsa::BigInt::MontgomeryModularMultiplication(a, b, n, md);
            if (res != res2)
                std::terminate();       //incorrect detected.
            a = res;
        }
        std::cout << "correctness test done" << std::endl;
        for (int i = 0; i < 100000; i++)
            res = (a * b) % n;
        std::cout << "traditional done" << std::endl;   //costs 5.2s
        for (int i = 0; i < 100000; i++)
            res = io::encrypt::rsa::BigInt::MontgomeryModularMultiplication(a, b, n, md);
        std::cout << "montgomery done" << std::endl;    //costs 6.2s. why so slow and unstable?
    }

    //aes test
    if (false)
    {
        std::string bs64s, bs64s2, bs64s3;
        io::encrypt::aes aes;
        aes.type = aes.AES256;
        aes.rand_key();
        aes.rand_iv();
        base64_mem bs64;
        bs64.Encode((const char*)aes.key, sizeof(aes.key), &bs64s);
        bs64.Encode((const char*)aes.get_iv().data(), io::encrypt::aes::block_len, &bs64s2);
        io::encrypt::aes::ivector_m iv;
        memcpy(iv, aes.get_iv().data(), sizeof(iv));
        std::string text = "hello world. aes encoding and decoding.48 block";
        aes.CTR_xcrypt(std::span<uint8_t>((uint8_t*)text.data(), text.size()));
        bs64.Encode(text.c_str(), text.size(), &bs64s3);
        for (int i = 0; i < 1000001; i++)
        {
            aes.set_iv(iv);
            aes.CTR_xcrypt(std::span<uint8_t>((uint8_t*)text.data(), text.size()));
        }
        std::cout << text << std::endl;
        //2s for 45.78MB data encrypt, single thread.
        //todo: use aesenc assembly.
    }

    //rsa test
    if (false)
    {
        std::string hex_pu, hex_pr, enc, dec = "hello world. 512 bits are so huge.";
        std::ostringstream oss;
        io::encrypt::rsa rsa;
        rsa.generate(1024);
        oss << rsa.public_key;
        hex_pu = oss.str();
        std::cout << hex_pu << "\n";
        oss = std::ostringstream();
        oss << rsa.private_key;
        hex_pr = oss.str();
        std::cout << hex_pr << "\n";

        io::encrypt::rsa::BigInt i,o;
        io::encrypt::rsa rsa_c;
        rsa_c.public_key = rsa.public_key;
        rsa_c.keyInit();                            //calculate the relative params about this public key.
        i.fromBytes(std::span<const char>(dec.c_str(), dec.size()));
        o = rsa_c.encryptByPu(i);
        oss = std::ostringstream();
        oss << o;
        enc = oss.str();
        std::cout << enc << "\n";

        std::string dec2;
        auto o2 = rsa.decodeByPuPr(o);
        o = rsa.encryptByPuPr(o2);
        o2.toBytes(dec2);
        std::cout << "decrypted pwd:" << dec2 << "\n";
    }

    //mem_testmain();

    //coroutine test
    io::ioManager::auto_go(10);     //10 threads
    io::ioManager::auto_once(test_tcp_server);
    for (int i = 0; i < 64; i++)
    {
        //io::ioManager::auto_once(test_tcp_client);
    }
    std::this_thread::sleep_for(std::chrono::years(30));

    return 0;
}