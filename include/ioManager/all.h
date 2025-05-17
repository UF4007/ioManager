#pragma once


// core
#include "ioManager.h"
#include "pipeline.h"
#include "timer.h"


// namespace io::sock -- hardware/system io socket
#if IO_USE_ASIO
#include "socket/asio/tcp.h"
#include "socket/asio/tcp_accp.h"
#include "socket/asio/udp.h"
#endif


// namespace io::prot -- software simulated protocols
#include "protocol/async_chan.h"
#include "protocol/async_semaphore.h"
#include "protocol/chan.h"
#include "protocol/packet_sentinel.h"

#include "protocol/kcp/kcp.h"
#include "protocol/http/http.h"
#include "protocol/mqtt/mqtt.h"
