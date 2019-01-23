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

class ManagerConnection {
public:	
	// --------- Nested class --------- //
	class IpAdress {
	public:
		// Constructors
		IpAdress(const std::string& ipAndPort);
		IpAdress(const std::string& ip, int port);
		IpAdress(char c1, char c2, char c3, char c4, int port);
		IpAdress(size_t target, int port);
		
		// Methods
		std::string toString() const;
		std::string toFullString() const;
		size_t toNumber() const;
		
		// Setters
		void setPort(int);
		
		// Getters
		int getPort() const;
		const std::vector<char>& getTarget() const;
		bool isValide() const;
		
		IpAdress operator+(int);
		
	private:
		// Methods
		bool _targetFromString(const std::string& path);
		bool _portFromString(const std::string& port);
		
		// Members
		std::vector<char> _target;
		int _port;
		bool _valide;
	};
	
	// --------- Main class --------- //
	// Constructors
	ManagerConnection();
	~ManagerConnection();
	
	// Methods
	bool initialize();
	std::shared_ptr<Server> createServer(const Socket::CONNECTION_TYPE type, const Socket::CONNECTION_MODE mode, const int port = 80, const int pending = 10) const;
	std::shared_ptr<Socket> connectTo(const Socket::CONNECTION_TYPE type, const Socket::CONNECTION_MODE mode, const std::string& ipAdress = "localhost", const int port = 80) const;
	std::vector<IpAdress> snif(const IpAdress& ipBeg, const IpAdress& ipEnd, int stopAfterNb = -1) const;
	IpAdress getMyDHCP() const; // Only IPv4 family yet.
	
	// Getters
	bool isInitialized() const;	
	
private:	
	// Members
	bool _initialized;
};

#endif
