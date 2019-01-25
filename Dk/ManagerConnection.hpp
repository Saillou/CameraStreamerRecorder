#ifndef MANAGER_CONNECTION_H
#define MANAGER_CONNECTION_H

#ifdef _WIN32
	#include <winsock2.h> // Socket
	#include <iphlpapi.h> // Adapter Adress
	#include <Ws2tcpip.h> // inet_ntop 
#endif

#ifdef __linux__ 
	#include <sys/types.h> 	// getifaddrs
	#include <ifaddrs.h>	// getifaddrs
#endif

#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <string>

#include "Socket.hpp"
#include "Server.hpp"

#include "IpAdress.hpp"

class ManagerConnection {
public:		
	// Constructors
	ManagerConnection();
	~ManagerConnection();
	
	// Methods
	bool initialize();
	std::shared_ptr<Server> createServer(const Socket::CONNECTION_TYPE type, const Socket::CONNECTION_MODE mode, const IpAdress& ipGateway, const int pending = 10) const;
	std::shared_ptr<Socket> connectTo(const Socket::CONNECTION_TYPE type, const Socket::CONNECTION_MODE mode, const IpAdress& ipGateway) const;
	std::vector<IpAdress> snif(const IpAdress& ipBeg, const IpAdress& ipEnd, int stopAfterNb = -1, std::vector<IpAdress> blackList = {}) const;
	
	static IpAdress getGatewayAdress(IpAdress::IP_ADRESS_TYPE ipType = IpAdress::IP_V4); // no ip v6 for ubuntu yet
	
	// Getters
	bool isInitialized() const;	
	
private:	
	// Members
	bool _initialized;
};

#endif
