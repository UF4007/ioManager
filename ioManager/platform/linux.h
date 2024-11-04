#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define ioctlsocket ::ioctl
#define closesocket ::close
#endif
//volunteerDriver
inline void io::volunteerDriver::selectDrive() {
	static fd_set read_fds, write_fds, except_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&except_fds);
	uint64_t fd_max = 0;

	//tcp
	while (select_rbt_busy.test_and_set(std::memory_order_acquire));
	if (select_rbt.size())
	{
		for (auto& [key, promise] : select_rbt) {
			if (promise.completable() == false)
				continue;
			FD_SET(key, &read_fds);
			if (promise.getPointer()->depleted == -1)
				FD_SET(key, &write_fds);
			FD_SET(key, &except_fds);
		}
		fd_max = select_rbt.rbegin()->first + 1;
	}
	select_rbt_busy.clear();

	//tcp server
	while (select_rbt_server_busy.test_and_set(std::memory_order_acquire));
	if (select_rbt_server.size())
	{
		for (auto& [key, promise] : select_rbt_server) {
			if (promise.completable() == false)
				continue;
			FD_SET(key, &read_fds);
			//FD_SET(key, &write_fds);
			//FD_SET(key, &except_fds);
		}
		fd_max = std::max(fd_max, select_rbt_server.rbegin()->first + 1);
	}
	select_rbt_server_busy.clear();

	//udp
	while (select_rbt_udp_busy.test_and_set(std::memory_order_acquire));
	if (select_rbt_udp.size())
	{
		for (auto& [key, promise] : select_rbt_udp) {
			if (promise.completable() == false)
				continue;
			FD_SET(key, &read_fds);
			//FD_SET(key, &write_fds);
			//FD_SET(key, &except_fds);
		}
		fd_max = std::max(fd_max, select_rbt_udp.rbegin()->first + 1);
	}
	select_rbt_udp_busy.clear();

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 100;	//fixed 100us per circle
	int result = select(fd_max, &read_fds, &write_fds, &except_fds, &timeout);

	if (result > 0) {
		//tcp client
		while (select_rbt_busy.test_and_set(std::memory_order_acquire));
		for (auto& [key, promise] : select_rbt) {
			if (FD_ISSET(key, &except_fds)) {
				promise.abort();
				continue;
			}
			if (FD_ISSET(key, &read_fds)) {
				socketData* data = promise.getPointer();
				if (data->depleted != -1)
				{
					if (data->depleted < data->capacity)
					{
						memset(data->data + data->depleted, 0, sizeof(data->data) - data->depleted);
						int bytesReceived = recv(key, data->data + data->depleted, sizeof(data->data) - data->depleted, 0);
						if (bytesReceived > 0) {
							data->depleted += bytesReceived;
						}
						else
							promise.abort();
					}
				}
				promise.complete();
			}
			if (FD_ISSET(key, &write_fds)) {
				promise.getPointer()->depleted = 0;
				promise.complete();
			}
		}
		select_rbt_busy.clear();

		//tcp server
		while (select_rbt_server_busy.test_and_set(std::memory_order_acquire));
		for (auto& [key, promise] : select_rbt_server) {
			if (FD_ISSET(key, &read_fds)) {
				tcp_client_socket* prom_client = promise.getPointer();
				prom_client->close();
				prom_client->handle = accept(key, nullptr, nullptr);
				volunteerDriver::socket_count++;
				coPromise<socketData> data(promise._base->mngr);
				data.getPointer()->depleted = -1;
				while (select_rbt_busy.test_and_set(std::memory_order_acquire));
				volunteerDriver::select_rbt.insert(std::pair<uint64_t, coPromise<socketData>>((uint64_t)prom_client->handle, data));
				select_rbt_busy.clear();
				promise.complete();
			}
		}
		select_rbt_server_busy.clear();

		//udp, raw socket
		while (select_rbt_udp_busy.test_and_set(std::memory_order_acquire));
		for (auto& [key, promise] : select_rbt_udp) {
			if (FD_ISSET(key, &read_fds)) {
				socketDataUdp* data = promise.getPointer();
				if (data->depleted < data->capacity)
				{
					socklen_t len = sizeof(data->addr);
					memset(data->data + data->depleted, 0, sizeof(data->data) - data->depleted);
					int bytesReceived = recvfrom(key, data->data + data->depleted, sizeof(data->data) - data->depleted, 0, (sockaddr*)&data->addr, &len);
					if (bytesReceived > 0) {
						data->depleted += bytesReceived;
					}
					else
						continue;
				}
				promise.complete();
			}
		}
		select_rbt_udp_busy.clear();
	}
}



//netif
inline void io::netif::global_startup() {
	if (_is_init_netif.test_and_set() == false)
	{
		volunteerDriver::socket_enable.test_and_set();
		std::thread(volunteerDriver::go).detach();
	}
}
inline void io::netif::global_cleanup() {
	volunteerDriver::socket_enable.clear();
	_is_init_netif.clear();
}
inline io::netif::netif()
{
	global_startup();
}
inline io::netif::~netif() {}
inline io::err io::netif::reload() {
	//todo something
	return io::err::ok;
}
inline io::err io::netif::next() {
	return io::err::ok;
}



//icmp
inline io::icmp_client_socket::icmp_client_socket() {
	netif::global_startup();
	handle = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (handle == INVALID_SOCKET)
		throw std::runtime_error("socket get failed.");
	else
	{
		volunteerDriver::socket_count++;
		volunteerDriver::socket_count.notify_all();
	}

	u_long mode = 1;
	ioctlsocket(handle, FIONBIO, &mode);
};
inline io::icmp_client_socket::icmp_client_socket(icmp_client_socket&& right) {
	handle = right.handle;
	right.handle = INVALID_SOCKET;
}
inline void io::icmp_client_socket::operator=(icmp_client_socket&& right) {
	handle = right.handle;
	right.handle = INVALID_SOCKET;
}
inline io::icmp_client_socket::~icmp_client_socket() {
	if (handle != INVALID_SOCKET) {
		while (volunteerDriver::select_rbt_udp_busy.test_and_set(std::memory_order_acquire));
		volunteerDriver::select_rbt_udp.erase((uint64_t)handle);
		volunteerDriver::select_rbt_udp_busy.clear(std::memory_order_release);
		closesocket(handle);
		handle = INVALID_SOCKET;
		volunteerDriver::socket_count--;
	}
}
inline io::err io::icmp_client_socket::sendPing(coPromise<socketDataUdp>& promise, const sockaddr_in& addr, std::chrono::microseconds overtime, uint16_t id) {
	promise.setTimeout(overtime);
	icmp_data head = icmp_data::ping_header(id);
	int bytesSent = sendto(handle, reinterpret_cast<char*>(&head), sizeof(head), 0,
		reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));

	if (bytesSent == SOCKET_ERROR) return io::err::failed;

	struct timeval recvTimeout;
	recvTimeout.tv_sec = overtime.count() / 1000000;
	recvTimeout.tv_usec = overtime.count() % 1000000;

	setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout));
	setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout));

	while (volunteerDriver::select_rbt_udp_busy.test_and_set(std::memory_order_acquire));
	volunteerDriver::select_rbt_udp.insert(std::pair<uint64_t, coPromise<socketDataUdp>>((uint64_t)handle, promise));
	volunteerDriver::select_rbt_udp_busy.clear(std::memory_order_release);

	_tp = std::chrono::steady_clock::now();
	return io::err::ok;
}
inline std::chrono::nanoseconds io::icmp_client_socket::getDelay() {
	return std::chrono::steady_clock::now() - _tp;
}



//tcp client
inline io::tcp_client_socket::tcp_client_socket() {
	netif::global_startup();
};
inline io::tcp_client_socket::tcp_client_socket(tcp_client_socket&& right) {
	handle = right.handle;
	right.handle = INVALID_SOCKET;
}
inline void io::tcp_client_socket::operator=(tcp_client_socket&& right) {
	handle = right.handle;
	right.handle = INVALID_SOCKET;
}
inline io::tcp_client_socket::~tcp_client_socket() {
	close();
}
inline io::err io::tcp_client_socket::open() {
	if (handle != INVALID_SOCKET)
		close();

	handle = socket(AF_INET, SOCK_STREAM, 0);

	if (handle == INVALID_SOCKET)
	{
		return io::err::failed;
	}
	else
	{
		volunteerDriver::socket_count++;
		volunteerDriver::socket_count.notify_all();
	}

	u_long mode = 1;
	ioctlsocket(handle, FIONBIO, &mode);
	return io::err::ok;
}
inline io::err io::tcp_client_socket::connect(coPromise<socketData>& promise, const sockaddr_in& addr) {
	if (handle == INVALID_SOCKET)
		return io::err::failed;

	promise.getPointer()->depleted = -1;
	::connect(handle, (sockaddr*)&addr, sizeof(addr));

	while (volunteerDriver::select_rbt_busy.test_and_set(std::memory_order_acquire));
	volunteerDriver::select_rbt.insert(std::pair<uint64_t, coPromise<socketData>>((uint64_t)handle, promise));
	volunteerDriver::select_rbt_busy.clear(std::memory_order_release);

	return io::err::ok;
}
inline io::coPromise<io::socketData> io::tcp_client_socket::findPromise() {
	if (handle == INVALID_SOCKET)
		return nullptr;
	while (volunteerDriver::select_rbt_busy.test_and_set(std::memory_order_acquire));
	auto iter = volunteerDriver::select_rbt.find(handle);
	if (iter != volunteerDriver::select_rbt.end())
	{
		volunteerDriver::select_rbt_busy.clear(std::memory_order_release);
		return iter->second;
	}
	volunteerDriver::select_rbt_busy.clear(std::memory_order_release);
	return nullptr;
}
inline void io::tcp_client_socket::close() {
	if (handle != INVALID_SOCKET) {
		while (volunteerDriver::select_rbt_busy.test_and_set(std::memory_order_acquire));
		volunteerDriver::select_rbt.erase((uint64_t)handle);
		volunteerDriver::select_rbt_busy.clear(std::memory_order_release);
		closesocket(handle);
		handle = INVALID_SOCKET;
		volunteerDriver::socket_count--;
	}
}
inline io::err io::tcp_client_socket::send(io::socketData& data) {
	return this->send(data.data, data.depleted);
}
inline io::err io::tcp_client_socket::send(const char* c, size_t s) {
	if (handle == INVALID_SOCKET)
		return io::err::failed;
	int result = ::send(handle, c, s, MSG_NOSIGNAL | MSG_DONTWAIT);
	if (result == SOCKET_ERROR)
		return io::err::failed;
	return io::err::ok;
}



//tcp server
inline io::tcp_server_socket::tcp_server_socket() {
	netif::global_startup();
};
inline io::tcp_server_socket::tcp_server_socket(tcp_server_socket&& right) {
	handle = right.handle;
	right.handle = INVALID_SOCKET;
}
inline void io::tcp_server_socket::operator=(tcp_server_socket&& right) {
	handle = right.handle;
	right.handle = INVALID_SOCKET;
}
inline io::tcp_server_socket::~tcp_server_socket() {
	close();
}
inline io::err io::tcp_server_socket::open() {
	if (handle != INVALID_SOCKET)
		close();

	handle = socket(AF_INET, SOCK_STREAM, 0);

	if (handle == INVALID_SOCKET)
	{
		return io::err::failed;
	}
	else
	{
		volunteerDriver::socket_count++;
		volunteerDriver::socket_count.notify_all();
	}

	u_long mode = 1;
	ioctlsocket(handle, FIONBIO, &mode);
	return io::err::ok;
}
inline io::err io::tcp_server_socket::bind(coPromise<tcp_client_socket>& promise, const sockaddr_in& addr) {
	if (handle == INVALID_SOCKET)
		return io::err::failed;

	if (::bind(handle, (sockaddr*)&addr, sizeof(addr)) < 0)
		return io::err::failed;

	if (::listen(handle, 1000) < 0)
		return io::err::failed;

	while (volunteerDriver::select_rbt_server_busy.test_and_set(std::memory_order_acquire));
	volunteerDriver::select_rbt_server.insert(std::pair<uint64_t, coPromise<tcp_client_socket>>((uint64_t)handle, promise));
	volunteerDriver::select_rbt_server_busy.clear(std::memory_order_release);

	return io::err::ok;
}
inline void io::tcp_server_socket::close() {
	if (handle != INVALID_SOCKET) {
		while (volunteerDriver::select_rbt_server_busy.test_and_set(std::memory_order_acquire));
		volunteerDriver::select_rbt_server.erase((uint64_t)handle);
		volunteerDriver::select_rbt_server_busy.clear(std::memory_order_release);
		closesocket(handle);
		handle = INVALID_SOCKET;
		volunteerDriver::socket_count--;
	}
}




//udp
inline io::udp_socket::udp_socket() {
	netif::global_startup();
};
inline io::udp_socket::udp_socket(udp_socket&& right) {
	handle = right.handle;
	right.handle = INVALID_SOCKET;
}
inline void io::udp_socket::operator=(udp_socket&& right) {
	handle = right.handle;
	right.handle = INVALID_SOCKET;
}
inline io::udp_socket::~udp_socket() {
	close();
}
inline io::err io::udp_socket::open() {
	if (handle != INVALID_SOCKET)
		close();

	handle = socket(AF_INET, SOCK_DGRAM, 0);

	if (handle == INVALID_SOCKET)
	{
		return io::err::failed;
	}
	else
	{
		volunteerDriver::socket_count++;
		volunteerDriver::socket_count.notify_all();
	}

	u_long mode = 1;
	ioctlsocket(handle, FIONBIO, &mode);
	return io::err::ok;
}
inline io::err io::udp_socket::bind(coPromise<socketDataUdp>& promise, const sockaddr_in& addr) {
	if (handle == INVALID_SOCKET)
		return io::err::failed;

	if (::bind(handle, (sockaddr*)&addr, sizeof(addr)) < 0)
		return io::err::failed;

	while (volunteerDriver::select_rbt_udp_busy.test_and_set(std::memory_order_acquire));
	volunteerDriver::select_rbt_udp.insert(std::pair<uint64_t, coPromise<socketDataUdp>>((uint64_t)handle, promise));
	volunteerDriver::select_rbt_udp_busy.clear(std::memory_order_release);

	return io::err::ok;
}
inline void io::udp_socket::close() {
	if (handle != INVALID_SOCKET) {
		while (volunteerDriver::select_rbt_udp_busy.test_and_set(std::memory_order_acquire));
		volunteerDriver::select_rbt_udp.erase((uint64_t)handle);
		volunteerDriver::select_rbt_udp_busy.clear(std::memory_order_release);
		closesocket(handle);
		handle = INVALID_SOCKET;
		volunteerDriver::socket_count--;
	}
}
inline io::err io::udp_socket::sendto(const char* c, size_t s, const sockaddr_in& addr) {
	if (handle == INVALID_SOCKET)
		return io::err::failed;
	int result = ::sendto(handle, c, s, MSG_NOSIGNAL | MSG_DONTWAIT, (sockaddr*)&addr, sizeof(addr));
	if (result == SOCKET_ERROR)
		return io::err::failed;
	return io::err::ok;
}
inline io::err io::udp_socket::sendtoAndBind(const char* c, size_t s, const sockaddr_in& addr, coPromise<socketDataUdp>& promise) {
	if (sendto(c, s, addr) == io::err::failed)
		return io::err::failed;

	while (volunteerDriver::select_rbt_udp_busy.test_and_set(std::memory_order_acquire));
	volunteerDriver::select_rbt_udp.insert(std::pair<uint64_t, coPromise<socketDataUdp>>((uint64_t)handle, promise));
	volunteerDriver::select_rbt_udp_busy.clear(std::memory_order_release);
	return io::err::ok;
}