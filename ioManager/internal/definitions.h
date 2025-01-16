//definitions



//awaiter
inline io::lowlevel::awaiter::~awaiter() {
	auto expected = awaiter::timing;
	if (this->status.compare_exchange_strong(expected, awaiter::idle))
	{
		while (mngr->spinLock_tm.test_and_set(std::memory_order_acquire));
		node.next->node.prev = node.prev;
		node.prev->node.next = node.next;
		mngr->spinLock_tm.clear(std::memory_order_release);
	}
}



// coPromise
template <typename _T>
inline io::coPromise<_T>::coPromise(coPromiseStack<_T> &stack) noexcept
{
    _base = &stack._awa;
    (_base->count)++;
}
template <typename _T>
inline io::coPromise<_T>::coPromise(coAsync<_T>&& async) noexcept
{
	_base = async._base;
	async._base = nullptr;
}
template <typename _T>
inline void io::coPromise<_T>::cdd() {
	if (_base)
	{
		if (_base->count.fetch_sub(1) == 1)
		{
			if constexpr (!std::is_same_v<_T, void>)
			{
				_T* mem2 = reinterpret_cast<_T*>(reinterpret_cast<char*>(_base) + sizeof(lowlevel::awaiter));
				mem2->~_T();
			}
			if (_base->status.load(std::memory_order_acquire) != lowlevel::awaiter::queueing)
			{
				lowlevel::awaiter* mem1 = _base;
				mem1->~awaiter();
				::operator delete(_base);
			}
		}
	}
}
template <typename _T>
template <typename ...Args>
inline io::coPromise<_T>::coPromise(ioManager* m, Args&&... consArgs) {
	if (m == nullptr)
	{
		_base = nullptr;
		return;
	}
	if constexpr (std::is_same_v<_T, void>)
	{
		_base = new lowlevel::awaiter(m);
	}
	else
	{
		_base = (lowlevel::awaiter*)::operator new(sizeof(lowlevel::awaiter) + sizeof(_T));
		new (_base)lowlevel::awaiter(m);
		new (reinterpret_cast<char*>(_base) + sizeof(lowlevel::awaiter))_T(std::forward<Args>(consArgs)...);
	}
}
template <typename _T>
inline io::coPromise<_T>::coPromise(const coPromise<_T>& right) noexcept
{
	_base = right._base;
	if (_base)
	{
		(_base->count)++;
	}
}
template <typename _T>
void io::coPromise<_T>::operator=(const coPromise<_T>& right) noexcept
{
	if (&right == this)
		return;
	cdd();
	_base = right._base;
	if (_base)
	{
		(_base->count)++;
	}
}
template <typename _T>
inline io::coPromise<_T>::coPromise(coPromise<_T> &&right) noexcept : _base(right._base)
{
    right._base = nullptr;
}
template <typename _T>
void io::coPromise<_T>::operator=(coPromise<_T> &&right) noexcept
{
    cdd();
    this->_base = right._base;
    right._base = nullptr;
}
template <typename _T>
inline io::coPromise<_T>::~coPromise() {
	cdd();
}
template <typename _T>
inline io::coPromise<_T>::operator bool() {
	return _base;
}
template <typename _T>
template <typename _Duration>
inline io::err io::coPromise<_T>::setTimeout(_Duration time)
{
	if (isSettled() == true)
		return io::err::failed;
	while (_base->mngr->spinLock_tm.test_and_set(std::memory_order_acquire));
	auto expected = lowlevel::awaiter::idle;
	if (_base->status.compare_exchange_strong(expected, lowlevel::awaiter::timing) == false)
	{
		_base->mngr->spinLock_tm.clear(std::memory_order_release);
		return io::err::failed;
	}

	lowlevel::awaiter* pos = _base->mngr->timeAwaiterCentral.prev;
	_base->timeout = std::chrono::steady_clock::now() + time;

	//find an appropriate timeline position
	while (1)
	{
		if (pos == (lowlevel::awaiter*)_base->mngr)
			break;
		if (pos->timeout <= _base->timeout)
			break;
		pos = pos->node.prev;
	}

	//add new awaiter in timeline
	lowlevel::awaiter** slot1 = &pos->node.next;
	lowlevel::awaiter** slot2 = &(pos->node.next->node.prev);
	_base->node.prev = pos;
	_base->node.next = pos->node.next;
	*slot1 = *slot2 = _base;

	_base->mngr->spinLock_tm.clear(std::memory_order_release);

	_base->mngr->suspend_sem.release();
	return io::err::ok;
}
template <typename _T>
inline void io::coPromise<_T>::complete_base()
{
	lowlevel::awaiter::_status expected = lowlevel::awaiter::timing, next;

	bool timer_execute_by_manager_thread_just_now = false;

	if (_base->set_lock.test_and_set(std::memory_order_acq_rel) == true)	//awaiting by certain coroutine
	{
		next = lowlevel::awaiter::queueing;
	}
	else																	//awaiter is idle
	{
		next = lowlevel::awaiter::idle;
	}

	if (_base->status.compare_exchange_strong(expected, next))
	{
		while (_base->mngr->spinLock_tm.test_and_set(std::memory_order_acquire));
		if (_base->node.prev)													//remove timing
		{
			_base->node.next->node.prev = _base->node.prev;
			_base->node.prev->node.next = _base->node.next;
			_base->node.prev = nullptr;
		}
		else
			timer_execute_by_manager_thread_just_now = true;
		_base->mngr->spinLock_tm.clear(std::memory_order_release);
	}

	if (next == lowlevel::awaiter::queueing)
	{
		if (timer_execute_by_manager_thread_just_now == false)
		{
			_base->status.store(lowlevel::awaiter::queueing, std::memory_order_release);
			while (_base->mngr->spinLock_rd.test_and_set(std::memory_order_acquire));
			_base->node.next = _base->mngr->readyAwaiter;
			_base->mngr->readyAwaiter = _base;
			_base->mngr->spinLock_rd.clear(std::memory_order_release);
		}
	}

	_base->mngr->suspend_sem.release();
}
template <typename _T>
inline void io::coPromise<_T>::complete_base_local()
{
	lowlevel::awaiter::_status expected = lowlevel::awaiter::timing, next;

	if (_base->set_lock.test_and_set(std::memory_order_acq_rel))
	{
		next = lowlevel::awaiter::queueing;
	}
	else
	{
		next = lowlevel::awaiter::idle;
	}

	if (_base->status.compare_exchange_strong(expected, lowlevel::awaiter::idle))
	{
		while (_base->mngr->spinLock_tm.test_and_set(std::memory_order_acquire));
		if (_base->node.prev)													//remove timing
		{
			_base->node.next->node.prev = _base->node.prev;
			_base->node.prev->node.next = _base->node.next;
			_base->node.prev = nullptr;
		}
		_base->mngr->spinLock_tm.clear(std::memory_order_release);
	}

	if (next == lowlevel::awaiter::queueing)
	{
		if (_base->coro)
			_base->coro.resume();
	}

	_base->mngr->suspend_sem.release();
}
template <typename _T>
inline void io::coPromise<_T>::reject() {
	_base->set_status = lowlevel::awaiter::reject;
	complete_base();
}
template <typename _T>
inline void io::coPromise<_T>::timeout() {
	_base->set_status = lowlevel::awaiter::timeouted;
	complete_base();
}
template <typename _T>
inline void io::coPromise<_T>::resolve() {
	_base->set_status = lowlevel::awaiter::resolve;
	complete_base();
}
template <typename _T>
inline void io::coPromise<_T>::rejectLocal() {
	_base->set_status = lowlevel::awaiter::reject;
	complete_base_local();
}
template <typename _T>
inline void io::coPromise<_T>::timeoutLocal() {
	_base->set_status = lowlevel::awaiter::timeouted;
	complete_base_local();
}
template <typename _T>
inline void io::coPromise<_T>::resolveLocal() {
	_base->set_status = lowlevel::awaiter::resolve;
	complete_base_local();
}
template <typename _T>
inline _T* io::coPromise<_T>::data() {
	if constexpr (std::is_same_v<_T, void>)
		return nullptr;
	return (_T*)(reinterpret_cast<char*>(_base) + sizeof(lowlevel::awaiter));
}
template <typename _T>
inline io::err io::coPromise<_T>::canOccupy() {
	if (this->_base->occupy_lock.test(std::memory_order_acquire) == true)
		return io::err::failed;
	return io::err::ok;
}
template <typename _T>
inline io::err io::coPromise<_T>::tryOccupy() {
	if (this->_base->occupy_lock.test_and_set(std::memory_order_seq_cst) == true)
		return io::err::failed;
	return io::err::ok;
}
template <typename _T>
inline void io::coPromise<_T>::unlockOccupy() {
	this->_base->occupy_lock.clear(std::memory_order_release);
}
template <typename _T>
inline void io::coPromise<_T>::reset() {
	assert(this->isOwned() == false ||
		!"awaiter ERROR: this awaiter is being owned by some other coroutine.");
	auto expected = lowlevel::awaiter::timing;
	if (_base->status.compare_exchange_strong(expected, lowlevel::awaiter::idle))
	{
		while (_base->mngr->spinLock_tm.test_and_set(std::memory_order_acquire));
		if (_base->node.prev)													//remove timing
		{
			_base->node.next->node.prev = _base->node.prev;
			_base->node.prev->node.next = _base->node.next;
		}
		_base->mngr->spinLock_tm.clear(std::memory_order_release);
	}
	else
		_base->status.store(lowlevel::awaiter::idle, std::memory_order_release);
	_base->node.prev = nullptr;
	_base->node.next = nullptr;
	_base->set_status = lowlevel::awaiter::resolve;
	_base->set_lock.clear(std::memory_order_release);
	_base->occupy_lock.clear(std::memory_order_release);
	return;
}
template <typename _T>
inline bool io::coPromise<_T>::isTiming() const
{
	return _base->status.load(std::memory_order_relaxed) == lowlevel::awaiter::timing;
}
template <typename _T>
inline bool io::coPromise<_T>::isOwned() const
{
	return _base->coro.operator bool() || _base->status.load(std::memory_order_relaxed) == lowlevel::awaiter::queueing;
}
template <typename _T>
inline bool io::coPromise<_T>::isSettled() const
{
	return _base->set_lock.test(std::memory_order_acquire) == true && !isOwned();
}
template <typename _T>
inline bool io::coPromise<_T>::isResolve() const
{
	return isSettled() && _base->set_status == lowlevel::awaiter::resolve;
}
template <typename _T>
inline bool io::coPromise<_T>::isTimeout() const
{
	return isSettled() && _base->set_status == lowlevel::awaiter::timeouted;
}
template <typename _T>
inline bool io::coPromise<_T>::isReject() const
{
	return isSettled() && _base->set_status == lowlevel::awaiter::reject;
}



//coPromiseStack
template <typename _T>
template <typename... Args>
inline io::coPromiseStack<_T>::coPromiseStack(ioManager *m, Args &&...consArgs) : _awa(m), _content(std::forward<Args>(consArgs)...) {}



//coAsync
template <typename _T>
template<typename ...Args>
inline io::coAsync<_T>::promise_type::promise_type(ioManager* m, Args&&... args) : prom(m) {}



//coSelector
template<typename T>
inline io::coSelector::subTask io::coSelector::subCoro(io::coSelector* mul, io::coSelector::subTask::promise_type* next, coPromise<T> promise)
{
	io::coSelector::subTask::promise_type* prom_t;
	co_yield prom_t;
	while (prom_t->_pmul)
	{
		co_await *promise;						//await tag 1
		if (prom_t->_pmul)
		{
			if (prom_t->_pmul->coro)
			{
				prom_t->_pmul->coro.resume();		//await tag 2
			}
			else
			{
				prom_t->_pending_next = prom_t->_pmul->pending;
				prom_t->_pmul->pending = prom_t;
				co_await std::suspend_always{};		//await tag 3
			}
		}
		else
			break;
	}
}
template <typename Last>
inline void io::coSelector::add(coPromise<Last>& last)
{
	subTask st = subCoro(this, this->first, last);
	this->first = st._prom;
}
template <typename First, typename ...Remain>
inline void io::coSelector::add(coPromise<First>& first, coPromise<Remain>&... remain)
{
	this->add(first);
	this->add(remain...);
}
template <typename ...Args>
inline io::coSelector::coSelector(coPromise<Args>&... arg)
{
	this->add(arg...);
}
inline io::coSelector::~coSelector()
{
	std::vector<subTask::promise_type*> pendingList;
	subTask::promise_type* ptr = this->pending;

	while (ptr)
	{
		pendingList.push_back(ptr);
		ptr = ptr->_pending_next;
	}

	ptr = this->first;
	while (ptr)
	{
		subTask::promise_type* i = ptr;
		ptr = ptr->_next;
		auto handle = std::coroutine_handle<subTask::promise_type>::from_promise(*i);
		i->_pmul = nullptr;
		if (std::find(pendingList.begin(), pendingList.end(), i) != pendingList.end())	//destroy subCoros which are awaiting tag 3
		{
			handle.resume();
		}
		else
		{
			if (i->_awa->coro == handle)												//destroy subCoros which are awaiting tag 1
			{
				lowlevel::awaiter* awa = i->_awa;
				handle.resume();
				awa->set_lock.clear(std::memory_order_release);
			}
			else																		//destroy subCoros which are awaiting tag 2
			{
				//do nothing...
			}
		}
	}
}
inline bool io::coSelector::processAllPending()
{
	assert(this->first != nullptr || !"cannot await a null coSelector.");
	subTask::promise_type* ptr = this->pending;
	this->pending = nullptr;

	while (ptr)
	{
		subTask::promise_type* i = ptr;
		ptr = ptr->_pending_next;
		i->_pending_next = nullptr;
		std::coroutine_handle<subTask::promise_type>::from_promise(*i).resume();
	}

	if (this->pending)
		return false;			//try to awaite one or more promise failed, coSelector cannot be await.
	else							//must handle and reset ALL promise to make them awaitable before multi_await.
		return true;
}
template <typename T>
inline io::err io::coSelector::remove(coPromise<T>& right)
{
	bool pending_found = false;

	subTask::promise_type* ptr = this->pending;
	subTask::promise_type** slot = &this->pending;

	while (ptr)
	{
		if (ptr->_awa == right._base)
		{
			*slot = ptr->_pending_next;
			pending_found = true;
			break;
		}
		slot = &ptr->_pending_next;
		ptr = ptr->_pending_next;
	}

	ptr = this->first;
	slot = &this->first;
	while (ptr)
	{
		if (ptr->_awa == right._base)
		{
			*slot = ptr->_next;
			auto handle = std::coroutine_handle<subTask::promise_type>::from_promise(*ptr);
			ptr->_pmul = nullptr;
			if (pending_found)																//destroy subCoros which are awaiting tag 3
			{
				handle.resume();
			}
			else
			{
				if (ptr->_awa->coro == handle)												//destroy subCoros which are awaiting tag 1
				{
					handle.resume();
					right._base->set_lock.clear(std::memory_order_release);
				}
				else																		//destroy subCoros which are awaiting tag 2
				{
					//do nothing...
				}
			}
			return io::err::ok;
		}
		slot = &ptr->_next;
		ptr = ptr->_next;
	}
	return io::err::failed;
}



//coDispatcher
template <typename coPromise_Type>
template<typename ...Args>
inline io::coDispatchedTask<coPromise_Type>::promise_type::promise_type(ioManager* m, Args&&... args) : prom(m) {}



//ioManager
inline io::ioManager::ioManager() {
	//timeAwaiterCentral.prev = (lowlevel::awaiter*)this;	// no, this is ub
	auto s = this;
	std::memcpy(&timeAwaiterCentral.next, &s, sizeof(this));
	std::memcpy(&timeAwaiterCentral.prev, &s, sizeof(this));
}
inline io::ioManager::~ioManager()
{
	this->stop(true);
};
inline void io::ioManager::once(coTask::processPtr ptr)
{
	this->suspend_sem.release();
	while (spinLock_pd.test_and_set(std::memory_order_acquire));
	pendingTask.push(ptr);
	spinLock_pd.clear(std::memory_order_release);
}
inline void io::ioManager::drive()
{
	while (1)   //pending task
	{
		while (spinLock_pd.test_and_set(std::memory_order_acquire));
		if (pendingTask.size())
		{
			auto i = pendingTask.front();
			pendingTask.pop();
			spinLock_pd.clear(std::memory_order_release);
			i({ this });
			continue;
		}
		spinLock_pd.clear(std::memory_order_release);
		break;
	}

	while (spinLock_rd.test_and_set(std::memory_order_acquire));
	lowlevel::awaiter* readys = readyAwaiter;
	readyAwaiter = nullptr;
	spinLock_rd.clear(std::memory_order_release);
	while (1)	//ready awaiter
	{
		if (readys != nullptr)
		{
			uint32_t expected = 0;
			auto next = readys->node.next;
			if (readys->count.compare_exchange_strong(expected, 0))
			{
				lowlevel::awaiter* mem1 = readys;
				mem1->~awaiter();
				::operator delete(readys);
			}
			else
			{
				readys->status = lowlevel::awaiter::idle;
				if (readys->coro)
					readys->coro.resume();
				else
					readys->set_lock.test_and_set(std::memory_order_relaxed);
			}
			readys = next;
			continue;
		}
		break;
	}

	std::chrono::steady_clock::time_point suspend_next;
	auto suspend_max_p = suspend_max + std::chrono::steady_clock::now();
	while (1)   //over time tasks
	{
		while (spinLock_tm.test_and_set(std::memory_order_acquire));
		auto sthis = this;
		if (std::memcmp(&timeAwaiterCentral.next, &sthis, sizeof(this)) != 0)
		{
			const auto& now = std::chrono::steady_clock::now();
			if (timeAwaiterCentral.next->timeout <= now)
			{
				lowlevel::awaiter* operat = timeAwaiterCentral.next;
				operat->node.next->node.prev = operat->node.prev;
				operat->node.prev->node.next = operat->node.next;
				operat->node.prev = nullptr;
				spinLock_tm.clear(std::memory_order_release);
				operat->status = lowlevel::awaiter::idle;
				operat->set_status = lowlevel::awaiter::timeouted;
				if (operat->coro)			//no need to lock bcs it is coroutine
					operat->coro.resume();
				else
					readys->set_lock.test_and_set(std::memory_order_relaxed);
				continue;
			}
			else
			{
				suspend_next = timeAwaiterCentral.next->timeout;
				if (suspend_next >= suspend_max_p)
				{
					suspend_next = suspend_max_p;
				}
			}
		}
		else
		{
			suspend_next = suspend_max_p;
		}
		spinLock_tm.clear(std::memory_order_release);
		break;
	}

	//suspend
	bool _nodiscard = suspend_sem.try_acquire_until(suspend_next);
}
inline void io::ioManager::go()
{
	assert(going == false || !"this ioManager had been launched.");
	going = true;
	isEnd.test_and_set(std::memory_order_acquire);
	std::thread([this] {
		while (going)
		{
			this->drive();
		}
		isEnd.clear(std::memory_order_release);
		}).detach();
}
inline void io::ioManager::stop(bool sync)
{
	going = false;
	if (sync)
	{
		while (isEnd.test_and_set(std::memory_order_acquire));
		isEnd.clear(std::memory_order_release);
	}
}
//ioManager Pool
inline int io::ioManager::getPendingFromAll()
{
	int i = getIndex.fetch_add(1);
	i %= all.size();
	std::atomic_flag* a = &all[i].spinLock_pd;
	while (a->test_and_set(std::memory_order_acquire));
	return i;
}
inline void io::ioManager::auto_go(uint32_t threadSum)
{
	assert(auto_going == false || !"repeat auto go");
	auto_going = true;
	for (int i = 0; i < threadSum; i++)
	{
		all.emplace_back();
		std::prev(all.end())->go();
	}
}
inline void io::ioManager::auto_stop() {
	all.clear();
	auto_going = false;
}
inline void io::ioManager::auto_once(coTask::processPtr ptr)
{
	int i = getPendingFromAll();
	all[i].pendingTask.push(ptr);
	all[i].spinLock_pd.clear(std::memory_order_release);
}