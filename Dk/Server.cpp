#include "Server.hpp"

// Constructors
Server::Server(const unsigned short port, const int maxPending) : 
	Socket("", port), 
	_maxPending(maxPending) 
{
	// Nothing else to do
}

Server::~Server() {
	closeAll();
}

// Methods
bool Server::initialize(const CONNECTION_TYPE type, const CONNECTION_MODE mode)	{
	_type = type;
	_mode = mode;
	
	// Type define id
	if(_type == NONE)
		return false;
	else if(_type == TCP)
		_idSocket = (int)socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	else if(_type == UDP)
		_idSocket = (int)socket(PF_INET, SOCK_DGRAM, 0);
	else {
		_type = NONE;
		std::cout << "[Servers] Type not recognized." << std::endl;
		return false;
	}
	
	// Create echo
	struct sockaddr_in  serverEcho;
	memset(&serverEcho, 0, sizeof(serverEcho));
	
	serverEcho.sin_family		= AF_INET;
	serverEcho.sin_addr.s_addr	= htonl(INADDR_ANY);
	serverEcho.sin_port			= htons(_port);	
	
	// Try bind
	if(bind(_idSocket, (struct sockaddr*)&serverEcho, sizeof(serverEcho)) == SOCKET_ERROR) {
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
	int clientId = -1;
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
#ifdef _WIN32
			int error = WSAGetLastError();
			
			switch(error) {
				case WSAEWOULDBLOCK: // Only triggered during not_blocking operations
					{ // Wait to be connected and check readable
						auto info = waitForAccess(ms);
						criticError = info.errorCode < 0;
					}
				break;
				
				default:
					std::cout << "Error not treated: " << error << std::endl;
				break;
			}
#endif
		}
		
		// Put back the socket in the defined mode
		if(ms > 0 && _mode == Socket::BLOCKING) {
			_changeMode(Socket::BLOCKING);
		}
	}
	else if(_type == UDP) {
		// What to do?
		// Nothing to wait ?
		// Read a message connect ? 
		criticError = true; // Not implemented yet.
	}
	
	// Everything is correct
	if(!criticError && clientId > 0) {		
		// Want same mode as server
		_changeMode(_mode, clientId);
		
		// Add to list
		_idScketsConnected.push_back(clientId);
	}
	
	return clientId;
}

void Server::closeSocket(int& idSocket) {
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






