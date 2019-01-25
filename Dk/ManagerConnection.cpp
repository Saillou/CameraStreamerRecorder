#include "ManagerConnection.hpp"

ManagerConnection::ManagerConnection() : _initialized(false) {
}
ManagerConnection::~ManagerConnection() {
#ifdef _WIN32
	WSACleanup();
#endif
}

// Methods
bool ManagerConnection::initialize() {
	// Avoid startup again
	if (_initialized)
		return _initialized;

#ifdef __linux__ 
	_initialized = true;
#elif _WIN32
	// Initialize "Winsock dll" version 2.2
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);

	int error = -1;
	if ((error = WSAStartup(wVersionRequested, &wsaData)) != 0)
		std::cout << "WSAStartup failed with error: " << error << "." << std::endl;
	else if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
		std::cout << "Could not find a usable version of Winsock.dll." << std::endl;
	else
		_initialized = true;
#else
	std::cout << "Not supported OS. - Only windows and ubuntu." << std::endl;
#endif

	return _initialized;
}


std::shared_ptr<Server> ManagerConnection::createServer(const Socket::CONNECTION_TYPE type, const Socket::CONNECTION_MODE mode, const IpAdress& ipGateway, const int pending) const {
	// Context ok ?
	if (!_initialized) {
		std::cout << "Manager not initialized." << std::endl;
		return nullptr;
	}

	// Initialize the server
	auto serv = std::make_shared<Server>(ipGateway, pending);
	if (!serv->initialize(type, mode))
		serv.reset();

	return serv;
}
std::shared_ptr<Socket> ManagerConnection::connectTo(const Socket::CONNECTION_TYPE type, const Socket::CONNECTION_MODE mode, const IpAdress& ipGateway = IpAdress::localhost(IpAdress::IP_V4)) const {
	// Context ok ?
	if (!_initialized) {
		std::cout << "Manager not initialized." << std::endl;
		return nullptr;
	}

	// Initialize the socket
	auto sock = std::make_shared<Socket>(ipGateway);
	if (!sock->initialize(type, mode))
		sock.reset();

	return sock;
}

// Getters
bool ManagerConnection::isInitialized() const {
	return _initialized;
}

std::vector<IpAdress> ManagerConnection::snif(const IpAdress& ipBeg, const IpAdress& ipEnd, int stopAfterNb, std::vector<IpAdress> blackList) const {
	std::vector<IpAdress> ipAvailable;

	// Determine delta port
	int pBeg = ipBeg.getPort();
	int pEnd = ipEnd.getPort();

	if (pBeg > pEnd) {
		pBeg += pEnd;
		pEnd = pBeg - pEnd;
		pBeg = pBeg - pEnd;
	}

	// Order ok?
	if (ipBeg > ipEnd) 
		return ipAvailable;

	// Iterate through all ip
	for (int port = pBeg; port <= pEnd; port++) {
		for (auto ipTarget = ipBeg; ipEnd > ipTarget; ipTarget = ipTarget + 1) {
			std::cout << " \t ... " << ipTarget.toFullString();

			if (!ipTarget.isValide())
				continue;

			// Avoid some ip
			bool avoidIp = false;
			for (auto it = blackList.begin(); it != blackList.end(); ++it) {
				if (*it == ipTarget) {
					avoidIp = true;
					break;
				}
			}
			if (avoidIp)
				continue;

			// Ping
			if (connectTo(Socket::TCP, Socket::NOT_BLOCKING, ipTarget) != nullptr) {
				ipAvailable.push_back(ipTarget);
				std::cout << " Ok \n";
				if (stopAfterNb > 0 && (int)ipAvailable.size() >= stopAfterNb)
					break;
			}
			else
				std::cout << " X \n";
		}
	}

	return ipAvailable;
}

IpAdress ManagerConnection::getGatewayAdress(IpAdress::IP_ADRESS_TYPE ipType) {
	IpAdress ipAdress(IpAdress::localhost(ipType)); // Init with a basic localhost
	auto family = ipType == IpAdress::IP_V4 ? AF_INET : AF_INET6;
	
#ifdef _WIN32
	// Helper
	auto adress2str = [](const SOCKET_ADDRESS &address) {
		std::string str = "";

		if (address.iSockaddrLength > 0) {
			if (address.lpSockaddr->sa_family == AF_INET) {
				sockaddr_in *si = (sockaddr_in *)(address.lpSockaddr);
				char a[INET_ADDRSTRLEN] = {};
				if (inet_ntop(AF_INET, &(si->sin_addr), a, sizeof(a)))
					str = std::string(a);
			}
			else if (address.lpSockaddr->sa_family == AF_INET6) {
				sockaddr_in6 *si = (sockaddr_in6 *)(address.lpSockaddr);
				char a[INET6_ADDRSTRLEN] = {};
				if (inet_ntop(AF_INET6, &(si->sin6_addr), a, sizeof(a)))
					str = std::string(a);
			}
		}

		return str;
	};

	ULONG outBufLen = 15000; 					// A 15 KB buffer
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL; 	// All interfaces adress

	pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);

	DWORD dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

	if (dwRetVal == NO_ERROR) {
		for (PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses; pCurrAddresses != NULL; pCurrAddresses = pCurrAddresses->Next) {
			bool found = false;
			for (auto ga = pCurrAddresses->FirstGatewayAddress; ga != NULL; ga = ga->Next) {
				ipAdress = IpAdress(adress2str(ga->Address), 0);
				if(ipAdress.isValide() && 
				   !ipAdress.isNull() && 
				   ipAdress != IpAdress::localhost(ipType)
				) break; // Don't want the local one if an other exists
			}
			if (found)
				break;
		}
	}

	if (pAddresses)
		free(pAddresses);
#else
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *temp_addr = NULL;
	std::vector<int> target((size_t)(ipType == IpAdress::IP_V4 ? 4 : 8), (int)0);

	// retrieve the current interfaces - 0 on success
	if (getifaddrs(&interfaces) == 0) {
		// Loop through linked list of interfaces
		temp_addr = interfaces;
		
		while (temp_addr != NULL) {
			if (temp_addr->ifa_addr->sa_family == family) {
				if(ipType == IpAdress::IP_V6) { // 16 uint8_t => 8 int
					uint8_t* addr = ((struct sockaddr_in6*)temp_addr->ifa_addr)->sin6_addr.s6_addr;
					for(int i = 0; i < target.size(); i++)
						target[i] = 256*(int)addr[2*i] + (int)addr[2*i+1];
				}
				else { // unsigned long => 4 uint8_t(uchar) => 4 int
					unsigned long addr = ((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr.s_addr;

					auto chRes = Protocole::BinMessage::Write_256(addr, 4);
					for(size_t i = 0; i < chRes.size(); i++)
						target[chRes.size() - i -1] = (int)(unsigned char)chRes[i];
				}
				
				ipAdress.fromTarget(target, 0);
				if(ipAdress.isValide() && 
				   !ipAdress.isNull() && 
				   ipAdress != IpAdress::localhost(ipType)
				  ) break; // Don't want the local one if an other exists	
			}
			temp_addr = temp_addr->ifa_next;
		}
	}

	freeifaddrs(interfaces);
#endif

	return ipAdress;
}
