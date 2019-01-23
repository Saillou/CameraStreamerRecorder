#include "Socket.hpp"

// Constructors
Socket::Socket(const std::string& ipAdress, const unsigned short port) :
	_ipAdress(ipAdress),
	_port(port),
	_idSocket(-1),
	_type(NONE)
{
	// Nothing else to do
}

Socket::~Socket() {
	if(_idSocket > 0) {
		shutdown(_idSocket, CLOSE_ER); // No emission or reception
#ifndef _MSC_VER  
		close(_idSocket);
#else
		closesocket(_idSocket);
#endif

		_idSocket = -1;
	}
}

// Methods
bool Socket::initialize(const CONNECTION_TYPE type, const CONNECTION_MODE mode)	{
	// Type define id
	_type = type;	
	_mode = mode;
	
	if(_type == NONE) 
		return false;
	else if(_type == TCP)
		_idSocket = static_cast<int>(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP));
	else if(_type == UDP) 
		_idSocket = static_cast<int>(socket(PF_INET, SOCK_DGRAM, 0));
	else {
		_type = NONE;
		std::cout << "[Socket] Type not recognized." << std::endl;
		return false;
	}
	
	// Create echo
	struct sockaddr_in clientEcho;
	memset(&clientEcho, 0, sizeof(clientEcho));
	
	clientEcho.sin_addr.s_addr	= inet_addr(_ipAdress.c_str());
	clientEcho.sin_port				= htons(_port);	
	clientEcho.sin_family		 	= AF_INET;
	
	// Use the mode defined
	_changeMode(mode);
	
	// Try to connect	
	if(connect(_idSocket, (struct sockaddr *)&clientEcho, sizeof(clientEcho)) == SOCKET_ERROR) {
		bool criticError = true;
		
#ifdef _WIN32
		int error = WSAGetLastError();
		
		switch(error) {
			case WSAEWOULDBLOCK: // Only triggered during not_blocking operations				
				{ // Wait to be connected and check writable
					auto info = waitForAccess(30);
					criticError = info.errorCode <= 0 || !info.writable;
				}
			break;
			
			case WSAETIMEDOUT:
				std::cout << "Server timeout." << std::endl;
			break;
			
			case WSAEISCONN:
				std::cout << "Socket is already connected." << std::endl;
				criticError = false;
			break;
			
			case WSAECONNREFUSED:
				std::cout << "Connection refused. " << std::endl;
			break;
			
			default:
				std::cout << "Error not treated: " << error << std::endl;
			break;
		}
#endif
		
		if(criticError) {
			return false;
		}
	}
	
	// Use the mode defined (may have been changed with accept() from server)
	_changeMode(mode);
	
	return true;
}
bool Socket::read(Protocole::BinMessage& msg, int idSocket) const {
	msg.clear();
	
	// Check
	if(_idSocket <= 0 || _type == NONE) {
		std::cout << "Socket not connected." << std::endl;
		return false;
	}
	
	if(idSocket <= 0)
		idSocket = _idSocket;
	
	if(_type == TCP) {
		// -- Read message --
		int received 		= -1;
		size_t messageSize = 0;
		size_t messageCode = 0;
		char* buffer 		=	nullptr;
		
		// Message size
		received = -1;
		buffer = (char*)realloc(buffer, Protocole::BinMessage::SIZE_SIZE * sizeof(char));
		
		if((received = recv(idSocket, buffer, Protocole::BinMessage::SIZE_SIZE, 0)) == (int)Protocole::BinMessage::SIZE_SIZE) {
			messageSize = Protocole::BinMessage::Read_256(buffer, Protocole::BinMessage::SIZE_SIZE);
		}
		else // Error size
			return false;

		// Message code
		received = -1;
		buffer = (char*)realloc(buffer, Protocole::BinMessage::SIZE_CODE * sizeof(char));
		
		if((received = recv(idSocket, buffer, Protocole::BinMessage::SIZE_CODE, 0)) == (int)Protocole::BinMessage::SIZE_CODE) {
			messageCode = Protocole::BinMessage::Read_256(buffer, Protocole::BinMessage::SIZE_CODE);
		}
		else // Error size
			return false;
		
		// Message data
		size_t already_read = 0;
		received 	= -1;
		buffer 	= (char*)realloc(buffer, messageSize * sizeof(char));
		
		while(already_read < messageSize) {
			size_t still = messageSize - already_read;
			if((received = recv(idSocket, buffer + already_read, (int)(still > BUFFER_SIZE_MAX ? BUFFER_SIZE_MAX : still), 0)) > 0) 
				already_read += received;
			else 
				break;
		}

		msg.set(messageCode, messageSize, buffer);
		free(buffer);

		// Result
		return msg.isValide();
	}
	if(_type == UDP) {
		// Not implemented yet
	}
	
	return false;
}
bool Socket::write(const Protocole::BinMessage& msg, int idSocket) const {
	// Check
	if(_idSocket <= 0 || _type == NONE) {
		std::cout << "Socket not connected." << std::endl;
		return false;
	}
	
	if(idSocket <= 0)
		idSocket = _idSocket;
	
	if(_type == TCP) {
		auto message = msg.serialize();
		int sended = (int)message.size();
		
		return (send(idSocket, message.data(), sended, 0) == sended);
	}
	if(_type == UDP) {
		// Not implemented yet
	}
	
	return false;
}
	
Socket::Accessiblity Socket::waitForAccess(unsigned long timeoutMs, int idSocket) const {
	Socket::Accessiblity access{false, false, 0};
	
	if(idSocket < 0) 
		idSocket = _idSocket;
	
	// Wait using select
	struct timeval timeout = {
		/* timeout.tv_sec = */(long)(timeoutMs / (long)1e3),
		/* timeout.tv_usec = */(long)(timeoutMs * (long)1e3) % (long)1e6
	};
	
	fd_set bkRead, bkWrite, bkErr;
	FD_ZERO(&bkRead);
	FD_ZERO(&bkWrite);
	FD_ZERO(&bkErr);
	
	FD_SET(idSocket, &bkRead);
	FD_SET(idSocket, &bkWrite);
	FD_SET(idSocket, &bkErr);
	
	access.errorCode = select(idSocket+1, &bkRead, &bkWrite, &bkErr, timeoutMs > 0 ? &timeout : NULL);
	if(access.errorCode > 0) { // No errors
		access.writable = FD_ISSET(idSocket, &bkWrite) > 0;
		access.readable = FD_ISSET(idSocket, &bkRead) > 0;
	}
	
	return access;
}

// Setters


// Getters
const std::string& Socket::getIpAdress() const {
	return _ipAdress;
}
const unsigned short& Socket::getPort() const {
	return _port;
}
const int& Socket::getId() const {
	return _idSocket;
}
const Socket::CONNECTION_TYPE& Socket::getType() const {
	return _type;
}


int Socket::_changeMode(const CONNECTION_MODE mode, int idSocket) {
	if(idSocket < 0)
		idSocket = this->_idSocket;
	
#ifdef __linux__ 
	// Use the standard POSIX 
	int oldFlags = fcntl(idSocket, F_GETFL, 0);
	int flags = (mode == NOT_BLOCKING) ? oldFlags | O_NONBLOCK : oldFlags & ~O_NONBLOCK;
	return fcntl(idSocket, F_SETFL, flags);
	
#elif _WIN32
	// Use the WSA 
	unsigned long ul = (mode == NOT_BLOCKING) ? 1 : 0; // Parameter for FIONBIO
	return ioctlsocket(idSocket, FIONBIO, &ul);
#endif

	// No implementation
	return -1;
}

