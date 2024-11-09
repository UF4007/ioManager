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



//coPromise
template <typename _T>
template <typename ...Args, typename U>
inline io::coPromise<_T>::promise_type::promise_type(ioManager* m, Args&&... consArgs) :prom(m, std::forward<Args>(consArgs)...) {}
template <typename _T>
template <typename ...Args2, typename U>
inline io::coPromise<_T>::promise_type::promise_type(ioManager* m, Args2... consArgs) : prom(m) {}
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
inline io::coPromise<_T>::coPromise(const coPromise<_T>& right) {
	_base = right._base;
	if (_base)
	{
		(_base->count)++;
	}
}
template <typename _T>
void io::coPromise<_T>::operator=(const coPromise<_T>& right) {
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
	if (isSet() == true)
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

	if (_base->set_lock.test_and_set(std::memory_order_acq_rel))
	{
		next = lowlevel::awaiter::queueing;
	}
	else
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
		_base->mngr->spinLock_tm.clear(std::memory_order_release);
	}

	if (next == lowlevel::awaiter::queueing)
	{
		while (_base->mngr->spinLock_rd.test_and_set(std::memory_order_acquire));
		_base->node.next = _base->mngr->readyAwaiter;
		_base->mngr->readyAwaiter = _base;
		_base->mngr->spinLock_rd.clear(std::memory_order_release);
	}

	_base->mngr->suspend_sem.release();
}
template <typename _T>
inline void io::coPromise<_T>::abort() {
	_base->set_status = lowlevel::awaiter::aborted;
	complete_base();
}
template <typename _T>
inline void io::coPromise<_T>::timeout() {
	_base->set_status = lowlevel::awaiter::timeouted;
	complete_base();
}
template <typename _T>
inline void io::coPromise<_T>::complete() {
	_base->set_status = lowlevel::awaiter::complete;
	complete_base();
}
template <typename _T>
inline _T* io::coPromise<_T>::getPointer() {
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
inline bool io::coPromise<_T>::reset() {
	assert(this->isOwned() == false ||
		!"awaiter ERROR: this awaiter is being owned by some other coroutine.");
	_base->set_lock.clear(std::memory_order_release);
	auto expected = lowlevel::awaiter::timing;
	if (_base->status.compare_exchange_strong(expected, lowlevel::awaiter::idle))
	{
		while (_base->mngr->spinLock_tm.test_and_set(std::memory_order_acquire));
		if (_base->node.prev)													//remove timing
		{
			_base->node.next->node.prev = _base->node.prev;
			_base->node.prev->node.next = _base->node.next;
			_base->node.prev = nullptr;
			_base->node.next = nullptr;
		}
		_base->mngr->spinLock_tm.clear(std::memory_order_release);
	}
	else
		_base->status.store(lowlevel::awaiter::idle, std::memory_order_release);
	_base->set_status = lowlevel::awaiter::complete;
	_base->occupy_lock.clear(std::memory_order_release);
	return true;
}
template <typename _T>
inline bool io::coPromise<_T>::isTiming()
{
	return _base->status.load(std::memory_order_relaxed) == lowlevel::awaiter::timing;
}
template <typename _T>
inline bool io::coPromise<_T>::isOwned()
{
	return _base->coro.operator bool() || _base->status.load(std::memory_order_relaxed) == lowlevel::awaiter::queueing;
}
template <typename _T>
inline bool io::coPromise<_T>::isSet()
{
	return _base->set_lock.test(std::memory_order_acquire) == true && !isOwned();
}
template <typename _T>
inline bool io::coPromise<_T>::isCompleted()
{
	return isSet() && _base->set_status == lowlevel::awaiter::complete;
}
template <typename _T>
inline bool io::coPromise<_T>::isTimeout()
{
	return isSet() && _base->set_status == lowlevel::awaiter::timeouted;
}
template <typename _T>
inline bool io::coPromise<_T>::isAborted()
{
	return isSet() && _base->set_status == lowlevel::awaiter::aborted;
}



//coMultiplex
template<typename T>
inline io::coMultiplex::subTask io::coMultiplex::subCoro(io::coMultiplex* mul, io::coMultiplex::subTask::promise_type* next, coPromise<T> promise)
{
	io::coMultiplex::subTask::promise_type* prom_t;
	co_yield prom_t;
	while (prom_t->_pmul)
	{
		task_await(promise);						//await tag 1
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
inline void io::coMultiplex::add(coPromise<Last>& last)
{
	subTask st = subCoro(this, this->first, last);
	this->first = st._prom;
}
template <typename First, typename ...Remain>
inline void io::coMultiplex::add(coPromise<First>& first, coPromise<Remain>&... remain)
{
	this->add(first);
	this->add(remain...);
}
template <typename ...Args>
inline io::coMultiplex::coMultiplex(coPromise<Args>&... arg)
{
	this->add(arg...);
}
inline io::coMultiplex::~coMultiplex()
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
inline bool io::coMultiplex::processAllPending()
{
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
		return false;			//try to awaite one or more promise failed, coMultiplex cannot be await.
	else							//must handle and reset ALL promise to make them awaitable before multi_await.
		return true;
}
template <typename T>
inline io::err io::coMultiplex::remove(coPromise<T>& right)
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



//ioManager
inline io::ioManager::ioManager() {
	timeAwaiterCentral.next = (lowlevel::awaiter*)this;
	timeAwaiterCentral.prev = (lowlevel::awaiter*)this;
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
		if (timeAwaiterCentral.next != (lowlevel::awaiter*)this)
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



//md5
inline io::encrypt::md5::md5(const char* byte, size_t size) {
	finished = false;
	/* Reset number of bits. */
	count[0] = count[1] = 0;
	/* Initialization constants. */
	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;

	/* Initialization the object according to message. */
	init((const std::byte*)byte, size);
}
inline io::encrypt::md5::md5(const std::string& message) {
	finished = false;
	/* Reset number of bits. */
	count[0] = count[1] = 0;
	/* Initialization constants. */
	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;

	/* Initialization the object according to message. */
	init((const std::byte*)message.c_str(), message.length());
}
/**
 * @Generate md5 digest.
 *
 * @return the message-digest.
 *
 */
std::span<const std::byte, 16> io::encrypt::md5::getDigest() {
	if (!finished) {
		finished = true;

		std::byte bits[8];
		uint32_t oldState[4];
		uint32_t oldCount[2];
		uint32_t index, padLen;

		/* Save current state and count. */
		memcpy(oldState, state, 16);
		memcpy(oldCount, count, 8);

		/* Save number of bits */
		encode(count, bits, 8);

		/* Pad out to 56 mod 64. */
		index = (uint32_t)((count[0] >> 3) & 0x3f);
		padLen = (index < 56) ? (56 - index) : (120 - index);
		init(PADDING, padLen);

		/* Append length (before padding) */
		init(bits, 8);

		/* Store state in digest */
		encode(state, digest, 16);

		/* Restore current state and count. */
		memcpy(state, oldState, 16);
		memcpy(count, oldCount, 8);
	}
	return digest;
}
/**
 * @Initialization the md5 object, processing another message block,
 * and updating the context.
 *
 * @param {input} the input message.
 *
 * @param {len} the number btye of message.
 *
 */
void io::encrypt::md5::init(const std::byte* input, size_t len) {

	uint32_t i, index, partLen;

	finished = false;

	/* Compute number of bytes mod 64 */
	index = (uint32_t)((count[0] >> 3) & 0x3f);

	/* update number of bits */
	if ((count[0] += ((uint32_t)len << 3)) < ((uint32_t)len << 3)) {
		++count[1];
	}
	count[1] += ((uint32_t)len >> 29);

	partLen = 64 - index;

	/* transform as many times as possible. */
	if (len >= partLen) {

		memcpy(&buffer[index], input, partLen);
		transform(buffer);

		for (i = partLen; i + 63 < len; i += 64) {
			transform(&input[i]);
		}
		index = 0;

	}
	else {
		i = 0;
	}

	/* Buffer remaining input */
	memcpy(&buffer[index], &input[i], len - i);
}
/**
 * @MD5 basic transformation. Transforms state based on block.
 *
 * @param {block} the message block.
 */
void io::encrypt::md5::transform(const std::byte block[64]) {

	uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	decode(block, x, 64);

	/* Round 1 */
	FF(a, b, c, d, x[0], s11, 0xd76aa478);
	FF(d, a, b, c, x[1], s12, 0xe8c7b756);
	FF(c, d, a, b, x[2], s13, 0x242070db);
	FF(b, c, d, a, x[3], s14, 0xc1bdceee);
	FF(a, b, c, d, x[4], s11, 0xf57c0faf);
	FF(d, a, b, c, x[5], s12, 0x4787c62a);
	FF(c, d, a, b, x[6], s13, 0xa8304613);
	FF(b, c, d, a, x[7], s14, 0xfd469501);
	FF(a, b, c, d, x[8], s11, 0x698098d8);
	FF(d, a, b, c, x[9], s12, 0x8b44f7af);
	FF(c, d, a, b, x[10], s13, 0xffff5bb1);
	FF(b, c, d, a, x[11], s14, 0x895cd7be);
	FF(a, b, c, d, x[12], s11, 0x6b901122);
	FF(d, a, b, c, x[13], s12, 0xfd987193);
	FF(c, d, a, b, x[14], s13, 0xa679438e);
	FF(b, c, d, a, x[15], s14, 0x49b40821);

	/* Round 2 */
	GG(a, b, c, d, x[1], s21, 0xf61e2562);
	GG(d, a, b, c, x[6], s22, 0xc040b340);
	GG(c, d, a, b, x[11], s23, 0x265e5a51);
	GG(b, c, d, a, x[0], s24, 0xe9b6c7aa);
	GG(a, b, c, d, x[5], s21, 0xd62f105d);
	GG(d, a, b, c, x[10], s22, 0x2441453);
	GG(c, d, a, b, x[15], s23, 0xd8a1e681);
	GG(b, c, d, a, x[4], s24, 0xe7d3fbc8);
	GG(a, b, c, d, x[9], s21, 0x21e1cde6);
	GG(d, a, b, c, x[14], s22, 0xc33707d6);
	GG(c, d, a, b, x[3], s23, 0xf4d50d87);
	GG(b, c, d, a, x[8], s24, 0x455a14ed);
	GG(a, b, c, d, x[13], s21, 0xa9e3e905);
	GG(d, a, b, c, x[2], s22, 0xfcefa3f8);
	GG(c, d, a, b, x[7], s23, 0x676f02d9);
	GG(b, c, d, a, x[12], s24, 0x8d2a4c8a);

	/* Round 3 */
	HH(a, b, c, d, x[5], s31, 0xfffa3942);
	HH(d, a, b, c, x[8], s32, 0x8771f681);
	HH(c, d, a, b, x[11], s33, 0x6d9d6122);
	HH(b, c, d, a, x[14], s34, 0xfde5380c);
	HH(a, b, c, d, x[1], s31, 0xa4beea44);
	HH(d, a, b, c, x[4], s32, 0x4bdecfa9);
	HH(c, d, a, b, x[7], s33, 0xf6bb4b60);
	HH(b, c, d, a, x[10], s34, 0xbebfbc70);
	HH(a, b, c, d, x[13], s31, 0x289b7ec6);
	HH(d, a, b, c, x[0], s32, 0xeaa127fa);
	HH(c, d, a, b, x[3], s33, 0xd4ef3085);
	HH(b, c, d, a, x[6], s34, 0x4881d05);
	HH(a, b, c, d, x[9], s31, 0xd9d4d039);
	HH(d, a, b, c, x[12], s32, 0xe6db99e5);
	HH(c, d, a, b, x[15], s33, 0x1fa27cf8);
	HH(b, c, d, a, x[2], s34, 0xc4ac5665);

	/* Round 4 */
	II(a, b, c, d, x[0], s41, 0xf4292244);
	II(d, a, b, c, x[7], s42, 0x432aff97);
	II(c, d, a, b, x[14], s43, 0xab9423a7);
	II(b, c, d, a, x[5], s44, 0xfc93a039);
	II(a, b, c, d, x[12], s41, 0x655b59c3);
	II(d, a, b, c, x[3], s42, 0x8f0ccc92);
	II(c, d, a, b, x[10], s43, 0xffeff47d);
	II(b, c, d, a, x[1], s44, 0x85845dd1);
	II(a, b, c, d, x[8], s41, 0x6fa87e4f);
	II(d, a, b, c, x[15], s42, 0xfe2ce6e0);
	II(c, d, a, b, x[6], s43, 0xa3014314);
	II(b, c, d, a, x[13], s44, 0x4e0811a1);
	II(a, b, c, d, x[4], s41, 0xf7537e82);
	II(d, a, b, c, x[11], s42, 0xbd3af235);
	II(c, d, a, b, x[2], s43, 0x2ad7d2bb);
	II(b, c, d, a, x[9], s44, 0xeb86d391);

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
}
/**
* @Encodes input (unsigned long) into output (byte).
*
* @param {input} usigned long.
*
* @param {output} byte.
*
* @param {length} the length of input.
*
*/
void io::encrypt::md5::encode(const uint32_t* input, std::byte* output, size_t length) {

	for (size_t i = 0, j = 0; j < length; ++i, j += 4) {
		output[j] = (std::byte)(input[i] & 0xff);
		output[j + 1] = (std::byte)((input[i] >> 8) & 0xff);
		output[j + 2] = (std::byte)((input[i] >> 16) & 0xff);
		output[j + 3] = (std::byte)((input[i] >> 24) & 0xff);
	}
}
/**
 * @Decodes input (byte) into output (usigned long).
 *
 * @param {input} bytes.
 *
 * @param {output} unsigned long.
 *
 * @param {length} the length of input.
 *
 */
void io::encrypt::md5::decode(const std::byte* input, uint32_t* output, size_t length) {
	for (size_t i = 0, j = 0; j < length; ++i, j += 4) {
		output[i] = ((uint32_t)input[j]) | (((uint32_t)input[j + 1]) << 8) |
			(((uint32_t)input[j + 2]) << 16) | (((uint32_t)input[j + 3]) << 24);
	}
}
/**
 * @Convert digest to string value.
 *
 * @return the hex string of digest.
 *
 */
std::string io::encrypt::md5::toStr() {
	const std::byte* digest_ = getDigest().data();
	std::string str;
	str.reserve(16 << 1);
	for (size_t i = 0; i < 16; ++i) {
		int t = (int)digest_[i];
		int a = t / 16;
		int b = t % 16;
		str.append(1, HEX_NUMBERS[a]);
		str.append(1, HEX_NUMBERS[b]);
	}
	return str;
}
inline uint32_t io::encrypt::md5::F(uint32_t x, uint32_t y, uint32_t z) {
	return ((x & y) | (~x & z));
}
inline uint32_t io::encrypt::md5::G(uint32_t x, uint32_t y, uint32_t z) {
	return ((x & z) | (y & ~z));
}
inline uint32_t io::encrypt::md5::H(uint32_t x, uint32_t y, uint32_t z) {
	return (x ^ y ^ z);
}
inline uint32_t io::encrypt::md5::I(uint32_t x, uint32_t y, uint32_t z) {
	return (y ^ (x | ~z));
}
/**
 * @Rotate Left.
 *
 * @param {num} the raw number.
 *
 * @param {n} rotate left n.
 *
 * @return the number after rotated left.
 */
inline uint32_t io::encrypt::md5::ROTATELEFT(uint32_t num, uint32_t n) {
	return ((num << n) | (num >> (32 - n)));
}
/**
 * @Transformations for rounds 1, 2, 3, and 4.
 */
inline void io::encrypt::md5::FF(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
	a += F(b, c, d) + x + ac;
	a = ROTATELEFT(a, s);
	a += b;
}
inline void io::encrypt::md5::GG(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
	a += G(b, c, d) + x + ac;
	a = ROTATELEFT(a, s);
	a += b;
}
inline void io::encrypt::md5::HH(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
	a += H(b, c, d) + x + ac;
	a = ROTATELEFT(a, s);
	a += b;
}
inline void io::encrypt::md5::II(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
	a += I(b, c, d) + x + ac;
	a = ROTATELEFT(a, s);
	a += b;
}



//aes
inline io::encrypt::aes::aes(key128 a) {
	::memcpy(key, a.data(), a.size());
}
inline io::encrypt::aes::aes(key192 a) {
	::memcpy(key, a.data(), a.size());
}
inline io::encrypt::aes::aes(key256 a) {
	::memcpy(key, a.data(), a.size());
}
inline void io::encrypt::aes::set_iv(ivector a) {
	::memcpy(Iv, a.data(), a.size());
}
inline io::encrypt::aes::ivector io::encrypt::aes::get_iv() {
	return Iv;
}
inline void io::encrypt::aes::rand_key() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dis(0, 255);

	int siz;
	switch (type)
	{
	case AES128:
		siz = 16;
		break;
	case AES192:
		siz = 24;
		break;
	case AES256:
		siz = 32;
		break;
	}
	for (int i = 0; i < siz; ++i) {
		key[i] = dis(gen);
	}
}
inline void io::encrypt::aes::rand_iv() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dis(0, 255);

	for (int i = 0; i < aes::block_len; ++i) {
		Iv[i] = dis(gen);
	}
}
inline void io::encrypt::aes::ECB_encrypt(std::span<uint8_t, 16> a) {
	KeyExpansion(RoundKey, key);
	Cipher((state_t*)a.data(), RoundKey);
}
inline void io::encrypt::aes::ECB_decrypt(std::span<uint8_t, 16> a) {
	KeyExpansion(RoundKey, key);
	InvCipher((state_t*)a.data(), RoundKey);
}
inline void io::encrypt::aes::CBC_encrypt(std::span<uint8_t> a) {
	KeyExpansion(RoundKey, key);
	size_t i;
	uint8_t* Iva = Iv;
	uint8_t* buf = a.data();
	for (i = 0; i < a.size(); i += aes::block_len)
	{
		XorWithIv(buf, Iva);
		Cipher((state_t*)buf, RoundKey);
		Iva = buf;
		buf += aes::block_len;
	}
	/* store Iv in ctx for next call */
	memcpy(Iv, Iva, aes::block_len);
}
inline void io::encrypt::aes::CBC_decrypt(std::span<uint8_t> a) {
	KeyExpansion(RoundKey, key);
	size_t i;
	uint8_t storeNextIv[aes::block_len];
	uint8_t* buf = a.data();
	for (i = 0; i < a.size(); i += aes::block_len)
	{
		memcpy(storeNextIv, buf, aes::block_len);
		InvCipher((state_t*)buf, RoundKey);
		XorWithIv(buf, Iv);
		memcpy(Iv, storeNextIv, aes::block_len);
		buf += aes::block_len;
	}
}
inline void io::encrypt::aes::CTR_xcrypt(std::span<uint8_t> a) {
	KeyExpansion(RoundKey, key);
	uint8_t buffer[aes::block_len];

	size_t i;
	int bi;
	uint8_t* buf = a.data();
	for (i = 0, bi = aes::block_len; i < a.size(); ++i, ++bi)
	{
		if (bi == aes::block_len) /* we need to regen xor compliment in buffer */
		{

			memcpy(buffer, Iv, aes::block_len);
			Cipher((state_t*)buffer, RoundKey);

			/* Increment Iv and handle overflow */
			for (bi = (aes::block_len - 1); bi >= 0; --bi)
			{
				/* inc will overflow */
				if (Iv[bi] == 255)
				{
					Iv[bi] = 0;
					continue;
				}
				Iv[bi] += 1;
				break;
			}
			bi = 0;
		}

		buf[i] = (buf[i] ^ buffer[bi]);
	}
}
inline uint32_t io::encrypt::aes::getNk()
{
	switch (type)
	{
	case AES128:
		return Nk128;
	case AES192:
		return Nk192;
	case AES256:
		return Nk256;
	}
}
inline uint32_t io::encrypt::aes::getNr()
{
	switch (type)
	{
	case AES128:
		return Nr128;
	case AES192:
		return Nr192;
	case AES256:
		return Nr256;
	}
}
inline uint8_t io::encrypt::aes::getSBoxInvert(uint8_t num)
{
	return rsbox[num];
}
inline uint8_t io::encrypt::aes::getSBoxValue(uint8_t num) {
	return sbox[num];
}
inline void io::encrypt::aes::KeyExpansion(uint8_t* RoundKey, const uint8_t* Key) {
	uint32_t Nk = getNk(), Nr = getNr();
	unsigned i, j, k;
	uint8_t tempa[4]; // Used for the column/row operations

	// The first round key is the key itself.
	for (i = 0; i < Nk; ++i)
	{
		RoundKey[(i * 4) + 0] = Key[(i * 4) + 0];
		RoundKey[(i * 4) + 1] = Key[(i * 4) + 1];
		RoundKey[(i * 4) + 2] = Key[(i * 4) + 2];
		RoundKey[(i * 4) + 3] = Key[(i * 4) + 3];
	}

	// All other round keys are found from the previous round keys.
	for (i = Nk; i < Nb * (Nr + 1); ++i)
	{
		{
			k = (i - 1) * 4;
			tempa[0] = RoundKey[k + 0];
			tempa[1] = RoundKey[k + 1];
			tempa[2] = RoundKey[k + 2];
			tempa[3] = RoundKey[k + 3];

		}

		if (i % Nk == 0)
		{
			// This function shifts the 4 bytes in a word to the left once.
			// [a0,a1,a2,a3] becomes [a1,a2,a3,a0]

			// Function RotWord()
			{
				const uint8_t u8tmp = tempa[0];
				tempa[0] = tempa[1];
				tempa[1] = tempa[2];
				tempa[2] = tempa[3];
				tempa[3] = u8tmp;
			}

			// SubWord() is a function that takes a four-byte input word and 
			// applies the S-box to each of the four bytes to produce an output word.

			// Function Subword()
			{
				tempa[0] = getSBoxValue(tempa[0]);
				tempa[1] = getSBoxValue(tempa[1]);
				tempa[2] = getSBoxValue(tempa[2]);
				tempa[3] = getSBoxValue(tempa[3]);
			}

			tempa[0] = tempa[0] ^ Rcon[i / Nk];
		}
		if (type == aes::AES256)
		{
			if (i % Nk == 4)
			{
				// Function Subword()
				{
					tempa[0] = getSBoxValue(tempa[0]);
					tempa[1] = getSBoxValue(tempa[1]);
					tempa[2] = getSBoxValue(tempa[2]);
					tempa[3] = getSBoxValue(tempa[3]);
				}
			}
		}
		j = i * 4; k = (i - Nk) * 4;
		RoundKey[j + 0] = RoundKey[k + 0] ^ tempa[0];
		RoundKey[j + 1] = RoundKey[k + 1] ^ tempa[1];
		RoundKey[j + 2] = RoundKey[k + 2] ^ tempa[2];
		RoundKey[j + 3] = RoundKey[k + 3] ^ tempa[3];
	}
}
inline void io::encrypt::aes::AddRoundKey(uint8_t round, state_t* state, const uint8_t* RoundKey) {
	for (uint8_t i = 0; i < 4; ++i) {
		for (uint8_t j = 0; j < 4; ++j) {
			(*state)[i][j] ^= RoundKey[(round * Nb * 4) + (i * Nb) + j];
		}
	}
}
inline void io::encrypt::aes::SubBytes(state_t* state) {
	for (uint8_t i = 0; i < 4; ++i) {
		for (uint8_t j = 0; j < 4; ++j) {
			(*state)[j][i] = getSBoxValue((*state)[j][i]);
		}
	}
}
inline void io::encrypt::aes::ShiftRows(state_t* state) {
	uint8_t temp;

	// Rotate first row 1 columns to left  
	temp = (*state)[0][1];
	(*state)[0][1] = (*state)[1][1];
	(*state)[1][1] = (*state)[2][1];
	(*state)[2][1] = (*state)[3][1];
	(*state)[3][1] = temp;

	// Rotate second row 2 columns to left  
	temp = (*state)[0][2];
	(*state)[0][2] = (*state)[2][2];
	(*state)[2][2] = temp;

	temp = (*state)[1][2];
	(*state)[1][2] = (*state)[3][2];
	(*state)[3][2] = temp;

	// Rotate third row 3 columns to left
	temp = (*state)[0][3];
	(*state)[0][3] = (*state)[3][3];
	(*state)[3][3] = (*state)[2][3];
	(*state)[2][3] = (*state)[1][3];
	(*state)[1][3] = temp;
}
inline uint8_t io::encrypt::aes::xtime(uint8_t x) {
	return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}
inline void io::encrypt::aes::MixColumns(state_t* state) {
	uint8_t i;
	uint8_t Tmp, Tm, t;
	for (i = 0; i < 4; ++i)
	{
		t = (*state)[i][0];
		Tmp = (*state)[i][0] ^ (*state)[i][1] ^ (*state)[i][2] ^ (*state)[i][3];
		Tm = (*state)[i][0] ^ (*state)[i][1]; Tm = xtime(Tm);  (*state)[i][0] ^= Tm ^ Tmp;
		Tm = (*state)[i][1] ^ (*state)[i][2]; Tm = xtime(Tm);  (*state)[i][1] ^= Tm ^ Tmp;
		Tm = (*state)[i][2] ^ (*state)[i][3]; Tm = xtime(Tm);  (*state)[i][2] ^= Tm ^ Tmp;
		Tm = (*state)[i][3] ^ t;              Tm = xtime(Tm);  (*state)[i][3] ^= Tm ^ Tmp;
	}
}
inline uint8_t io::encrypt::aes::Multiply(uint8_t x, uint8_t y) {
	return (((y & 1) * x) ^
		((y >> 1 & 1) * xtime(x)) ^
		((y >> 2 & 1) * xtime(xtime(x))) ^
		((y >> 3 & 1) * xtime(xtime(xtime(x)))) ^
		((y >> 4 & 1) * xtime(xtime(xtime(xtime(x)))))); /* this last call to xtime() can be omitted */
}
inline void io::encrypt::aes::InvMixColumns(state_t* state) {
	int i;
	uint8_t a, b, c, d;
	for (i = 0; i < 4; ++i)
	{
		a = (*state)[i][0];
		b = (*state)[i][1];
		c = (*state)[i][2];
		d = (*state)[i][3];

		(*state)[i][0] = Multiply(a, 0x0e) ^ Multiply(b, 0x0b) ^ Multiply(c, 0x0d) ^ Multiply(d, 0x09);
		(*state)[i][1] = Multiply(a, 0x09) ^ Multiply(b, 0x0e) ^ Multiply(c, 0x0b) ^ Multiply(d, 0x0d);
		(*state)[i][2] = Multiply(a, 0x0d) ^ Multiply(b, 0x09) ^ Multiply(c, 0x0e) ^ Multiply(d, 0x0b);
		(*state)[i][3] = Multiply(a, 0x0b) ^ Multiply(b, 0x0d) ^ Multiply(c, 0x09) ^ Multiply(d, 0x0e);
	}
}
inline void io::encrypt::aes::InvSubBytes(state_t* state) {
	uint8_t i, j;
	for (i = 0; i < 4; ++i)
	{
		for (j = 0; j < 4; ++j)
		{
			(*state)[j][i] = getSBoxInvert((*state)[j][i]);
		}
	}
}
inline void io::encrypt::aes::InvShiftRows(state_t* state) {
	uint8_t temp;

	// Rotate first row 1 columns to right  
	temp = (*state)[3][1];
	(*state)[3][1] = (*state)[2][1];
	(*state)[2][1] = (*state)[1][1];
	(*state)[1][1] = (*state)[0][1];
	(*state)[0][1] = temp;

	// Rotate second row 2 columns to right 
	temp = (*state)[0][2];
	(*state)[0][2] = (*state)[2][2];
	(*state)[2][2] = temp;

	temp = (*state)[1][2];
	(*state)[1][2] = (*state)[3][2];
	(*state)[3][2] = temp;

	// Rotate third row 3 columns to right
	temp = (*state)[0][3];
	(*state)[0][3] = (*state)[1][3];
	(*state)[1][3] = (*state)[2][3];
	(*state)[2][3] = (*state)[3][3];
	(*state)[3][3] = temp;
}
inline void io::encrypt::aes::Cipher(state_t* state, const uint8_t* RoundKey) {
	uint32_t Nr = getNr();
	uint8_t round = 0;

	// Add the First round key to the state before starting the rounds.
	AddRoundKey(0, state, RoundKey);

	// There will be Nr rounds.
	// The first Nr-1 rounds are identical.
	// These Nr rounds are executed in the loop below.
	// Last one without MixColumns()
	for (round = 1; ; ++round)
	{
		SubBytes(state);
		ShiftRows(state);
		if (round == Nr) {
			break;
		}
		MixColumns(state);
		AddRoundKey(round, state, RoundKey);
	}
	// Add round key to last round
	AddRoundKey(Nr, state, RoundKey);
}
inline void io::encrypt::aes::InvCipher(state_t* state, const uint8_t* RoundKey) {
	uint32_t Nr = getNr();
	uint8_t round = 0;

	// Add the First round key to the state before starting the rounds.
	AddRoundKey(Nr, state, RoundKey);

	// There will be Nr rounds.
	// The first Nr-1 rounds are identical.
	// These Nr rounds are executed in the loop below.
	// Last one without InvMixColumn()
	for (round = (Nr - 1); ; --round)
	{
		InvShiftRows(state);
		InvSubBytes(state);
		AddRoundKey(round, state, RoundKey);
		if (round == 0) {
			break;
		}
		InvMixColumns(state);
	}
}
inline void io::encrypt::aes::XorWithIv(uint8_t* buf, const uint8_t* Iv) {
	uint8_t i;
	for (i = 0; i < aes::block_len; ++i) // The block in AES is always 128bit no matter the key size
	{
		buf[i] ^= Iv[i];
	}
}



//rsa
inline std::deque<io::encrypt::rsa::BigInt> io::encrypt::rsa::BigInt::memPool = {};
inline std::stack<io::encrypt::rsa::BigInt*> io::encrypt::rsa::BigInt::memFreed = {};
inline io::encrypt::rsa::BigInt io::encrypt::rsa::BigInt::Zero(0);
inline io::encrypt::rsa::BigInt io::encrypt::rsa::BigInt::One(1);
inline io::encrypt::rsa::BigInt io::encrypt::rsa::BigInt::Two(2);
inline void io::encrypt::rsa::generate(unsigned int n)
{
	n /= 2;
	std::random_device rd;

	BigInt _p = createPrime(n, 10, rd);
	BigInt _q = createPrime(n, 10, rd);

	N = _p * _q;

	BigInt _ol = (_p - (uint32_t)1) * (_q - (uint32_t)1);

	createExp(_ol);
	keyInit();
}
inline io::encrypt::rsa::BigInt io::encrypt::rsa::createOddNum(unsigned int n, std::random_device& rd)
{
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(0, 15);
	n = n / 4;
	static unsigned char hex_table[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
	if (n)
	{
		std::ostringstream oss;
		for (std::size_t i = 0; i < n - 1; ++i)
			oss << hex_table[dis(gen)];
		oss << hex_table[1];
		std::string str(oss.str());
		return BigInt(str);
	}
	else
		return BigInt::Zero;
}
inline bool io::encrypt::rsa::isPrime(const BigInt& n, const unsigned int k, io::encrypt::rsa::BigInt& buffer)
{
	assert(n != BigInt::Zero);
	if (n == BigInt::Two)
		return true;

	BigInt n_1 = n - 1;
	BigInt::bit b(n_1);
	if (b.at(0) == 1)
		return false;

	//mon_domain md = n;
	//md.respawn();
	for (std::size_t t = 0; t < k; ++t)
	{
		BigInt a = createRandomSmallThan(n_1);
		buffer.clear(1);
		BigInt& d = buffer;
		for (int i = b.size() - 1; i >= 0; --i)
		{
			BigInt x = d;
			d = (d * d) % n;
			//d = BigInt::MontgomeryModularMultiplication(d, d, n, md);
			if (d == BigInt::One && x != BigInt::One && x != n_1)
				return false;

			if (b.at(i))
			{
				assert(d != BigInt::Zero);
				d = (a * d) % n;
				//d = BigInt::MontgomeryModularMultiplication(a, d, n, md);
			}
		}
		if (d != BigInt::One)
			return false;
	}
	return true;
}
inline io::encrypt::rsa::BigInt io::encrypt::rsa::createRandomSmallThan(const BigInt& a)
{
	unsigned long t = 0;
	do
	{
		t = rand();
	} while (t == 0);

	BigInt mod(t);
	BigInt r = mod % a;
	if (r == BigInt::Zero)
		r = a - BigInt::One;
	return r;
}
inline io::encrypt::rsa::BigInt io::encrypt::rsa::createPrime(unsigned int n, int it_count, std::random_device& rd)
{
	assert(it_count > 0);
	BigInt res = createOddNum(n, rd);		//create a random odd number
	BigInt buffer(BigInt::One);
	while (!isPrime(res, it_count, buffer))
	{
		while (1)
		{
			res.add(BigInt::Two);
			bool isPass = true;
			constexpr size_t ptable_size = sizeof(BigInt::prime_table) / sizeof(uint32_t);
			int ptable_max = std::min((const size_t)n / 3, ptable_size);		//experience formula
			for (int i = 0; i < ptable_max; i++)
			{
				if (!(res % (uint32_t)BigInt::prime_table[i]))
				{
					isPass = false;
					break;
				}
			}
			if (isPass)
				break;
		}
	}
	return res;
}
inline void io::encrypt::rsa::createExp(const BigInt& ou)
{
	_d = e.extendEuclid(ou);
}
inline io::encrypt::rsa::BigInt io::encrypt::rsa::encryptByPu(const BigInt& m)
{
	return m.moden(e, N);
	//return m.modenMon(e, N, mond);
}
inline io::encrypt::rsa::BigInt io::encrypt::rsa::decodeByPuPr(const BigInt& c)
{
	return c.moden(_d, N);
	//return c.modenMon(_d, N, mond);
}
inline io::encrypt::rsa::BigInt io::encrypt::rsa::encryptByPuPr(const BigInt& m)
{
	return decodeByPuPr(m);
}
inline io::encrypt::rsa::BigInt io::encrypt::rsa::decodeByPu(const BigInt& c)
{
	return encryptByPu(c);
}
inline void io::encrypt::rsa::keyInit() { this->mond.respawn(); }



//volunteerDrive
inline void io::volunteerDriver::go() {
	while (socket_enable.test_and_set(std::memory_order_acquire))
	{
		if (socket_count.load() > 0)
		{
			//select
			selectDrive();
		}
		else
		{
			socket_count.wait(0, std::memory_order_acquire);
		}
	}
}