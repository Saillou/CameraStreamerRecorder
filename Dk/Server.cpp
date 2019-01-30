#include "Server.hpp"

// Constructors
Server::Server(const IpAdress& ipGateway, const int maxPending) :
Socket(ipGateway),
	_maxPending(maxPending) 
{
	// Nothing else to do
}

Server::~Server() {
	closeAll();
}

// Methods
bool Server::initialize(const CONNECTION_TYPE type, const CONNECTION_MODE mode)	{
	// Init
	bool criticError = !_initializeSocketId(type, mode);
	if (criticError)
		return false;

	// Create echo
	struct sockaddr* serverEcho;
	size_t size = 0;

	if (_ipType == IpAdress::INVALID_IP) {
		return false;
	}
	else if (_ipType == IpAdress::IP_V4) {
		struct sockaddr_in serverEchoV4;
		memset(&serverEchoV4, 0, sizeof(serverEchoV4));
		
		serverEchoV4.sin_family = AF_INET;
		serverEchoV4.sin_addr.s_addr = htonl(INADDR_ANY);
		serverEchoV4.sin_port = htons(_ipGateway.getPort());
		
		serverEcho = (struct sockaddr*)&serverEchoV4;
		size = sizeof(serverEchoV4);
	}
	else if (_ipType == IpAdress::IP_V6) {
		struct sockaddr_in6 serverEchoV6;
		memset(&serverEchoV6, 0, sizeof(serverEchoV6));
		
		serverEchoV6.sin6_family = AF_INET6;
		serverEchoV6.sin6_addr = in6addr_any;
		serverEchoV6.sin6_port = htons(_ipGateway.getPort());
		
		serverEcho = (struct sockaddr*)&serverEchoV6;
		size = sizeof(serverEchoV6);
	}
	
	// Timeout
#ifdef __linux__ 
		struct timeval tvWait;
		tvWait.tv_sec = 1;
		tvWait.tv_usec = 0;
		setsockopt(_idSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tvWait, sizeof(tvWait));
		setsockopt(_idSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tvWait, sizeof(tvWait));
#else
		DWORD timeout = 1 * 1000;
		setsockopt(_idSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		setsockopt(_idSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#endif
	
	// Try bind
	if (bind(_idSocket, serverEcho, size) == SOCKET_ERROR) {
		std::cout << "Could not bind adress." << std::endl;
		return false;
	}
	
	// Listen
	if(_type == TCP) {
		if(listen(_idSocket, _maxPending) == SOCKET_ERROR) {
			std::cout << "Could not listen adress." << std::endl;
			return false;
		}
	}
	
	return true;
}

int Server::waitClient(long ms) {
	int clientId = 0;
	bool criticError = false;
	
	if(_idSocket <= 0 || _type == NONE) {
		std::cout << " Server not initialized" << std::endl;
		criticError = true;
	}
	else if(_type == TCP) {
		struct sockaddr_in clientEcho;
		SOCKET_LENGTH len = sizeof(clientEcho);
			
		if(ms > 0) {
			// Need the socket in non blocking mode to be able to timeout
			_changeMode(Socket::NOT_BLOCKING);
		}
		
		if((clientId = (int)accept(_idSocket, (struct sockaddr*)&clientEcho, &len)) == SOCKET_ERROR) {
			criticError = true;
			
			// Maybe we can solve the error
#ifdef __linux__ 
			int error = errno;
			const int errorCode = EWOULDBLOCK;
#else	
			int error = WSAGetLastError();	
			const int errorCode = WSAEWOULDBLOCK;
#endif
			switch(error) {
				case errorCode: // Only triggered during not_blocking operations
					{ // Wait to be connected and check readable
						auto info = waitForAccess(ms);
						criticError = info.errorCode < 0;
					}
				break;
				
				default:
					std::cout << "Error not treated: " << error << std::endl;
				break;
			}	
			
			errno = 0;		
		}
		
		// Put back the socket in the defined mode
		if(ms > 0 && _mode == Socket::BLOCKING) {
			_changeMode(Socket::BLOCKING);
		}
		
		if(clientId <= 0)
			criticError = true;
	}
	else if(_type == UDP) {
		// What to do?
		// Nothing to wait ?
		// Read a message connect ? 
		criticError = true; // Not implemented yet.
	}
	
	// Everything is correct
	if(!criticError) {		
		// Want same mode as server
		_changeMode(_mode, clientId);
		
		// Add to list
		_idScketsConnected.push_back(clientId);
	}
	
	return criticError ? 0 : clientId;
}

int Server::nextClient() const {
	if(_idScketsConnected.empty())
		return 0;
	else
		return _idScketsConnected[0];
}

void Server::closeSocket(int& idSocket) {
	std::cout << "Close socket" << idSocket << std::endl;
	if(idSocket > 0) {
		// Remove from list
		for(size_t i = 0; i < _idScketsConnected.size(); i++) {
			if(_idScketsConnected[i] == idSocket) {
				_idScketsConnected.erase(_idScketsConnected.begin() + i);
				break;
			}
		}
		
		shutdown(idSocket, CLOSE_ER); // No emission or reception
#ifndef _MSC_VER  
		close(idSocket);
#else
		closesocket(idSocket);
#endif

		idSocket = -1;
	}
}

void Server::closeAll() {
	for(auto& idSocket: _idScketsConnected)
		closeSocket(idSocket);
}



// ------------------------ ThreadWrite --------------------- //
Server::ThreadWrite::ThreadWrite(std::shared_ptr<Server> server, const Protocole::BinMessage& msg, const int idClient) :
	_ptrThread(new std::thread(&ThreadWrite::write, this, server, msg, idClient))
{
}

Server::ThreadWrite::~ThreadWrite() {
	if(_ptrThread)
		if(_ptrThread->joinable())
			_ptrThread->join();
		
	delete _ptrThread;
}
	
void Server::ThreadWrite::write(std::shared_ptr<Server> server, const Protocole::BinMessage& msg, const int idClient) {
	server->write(msg, idClient);
	_atomActive = false;
}
	
bool Server::ThreadWrite::isActive() {
	return _atomActive;
}






