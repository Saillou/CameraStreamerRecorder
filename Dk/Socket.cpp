#include "Socket.hpp"

// Constructors
Socket::Socket(const IpAdress& ipGateway) :
	_ipGateway(ipGateway),
	_idSocket(-1),
	_type(NONE),
	_ipType(ipGateway.getType())
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
	// Init
	bool success = _initializeSocketId(type, mode);
	if (!success)
		return false;

	// Create echo
	struct sockaddr* clientEcho;
	size_t size = 0;

	if (_ipType == IpAdress::INVALID_IP) {
		return false;
	}
	else if (_ipType == IpAdress::IP_V4) {
		struct sockaddr_in clientEchoV4;
		memset(&clientEchoV4, 0, sizeof(clientEchoV4));
		
		clientEchoV4.sin_family 		= AF_INET;
		clientEchoV4.sin_addr.s_addr 	= inet_addr(_ipGateway.toString().c_str());
		clientEchoV4.sin_port 			= htons(_ipGateway.getPort());
		
		clientEcho = (struct sockaddr*)&clientEchoV4;
		size = sizeof(clientEchoV4);
	}
	else if (_ipType == IpAdress::IP_V6) {
		struct sockaddr_in6 clientEchoV6;
		memset(&clientEchoV6, 0, sizeof(clientEchoV6));
		
		clientEchoV6.sin6_family = AF_INET6;
		memcpy(clientEchoV6.sin6_addr.s6_addr, _ipGateway.getChars().data(), 16);
		clientEchoV6.sin6_port = htons(_ipGateway.getPort());
		
		clientEcho = (struct sockaddr*)&clientEchoV6;
		size = sizeof(clientEchoV6);
	}
	
	// Use the mode defined
	_changeMode(mode);
	
	// Try to connect	
	if (connect(_idSocket, clientEcho, size) == SOCKET_ERROR) {
		success = false;
		
#ifdef _WIN32
		int error = WSAGetLastError();
		
		switch(error) {
			case WSAEWOULDBLOCK: // Only triggered during not_blocking operations				
				{ // Wait to be connected and check writable
					auto info = waitForAccess(30);
					success = info.errorCode > 0 && info.writable;
				}
			break;
			
			case WSAETIMEDOUT:
				std::cout << "Server timeout." << std::endl;
			break;
			
			case WSAEISCONN:
				std::cout << "Socket is already connected." << std::endl;
				success = true;
			break;
			
			case WSAECONNREFUSED:
				std::cout << "Connection refused. " << std::endl;
			break;
			
			default:
				std::cout << "Error not treated: " << error << std::endl;
			break;
		}
#endif
		
		if (!success)
			return false;
	}
	
	// Use the mode defined (may have been changed with accept() from server)
	_changeMode(mode);
	
	return success;
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
const IpAdress& Socket::getIpAdress() const {
	return _ipGateway;
}
const int& Socket::getId() const {
	return _idSocket;
}
const Socket::CONNECTION_TYPE& Socket::getType() const {
	return _type;
}


// Protected
bool Socket::_initializeSocketId(const CONNECTION_TYPE type, const CONNECTION_MODE mode) {
	bool success = true;

	// Type define id
	_type = type;
	_mode = mode;

	// Check ip
	if (!_ipGateway.isValide() || _ipGateway.isNull() || _ipType == IpAdress::INVALID_IP)
		_type = NONE;

	// Create socket	
	auto protoFam = _ipType == IpAdress::IP_V4 ? PF_INET : PF_INET6;

	if (_type == NONE)
		success = false;
	else if (_type == TCP)
		_idSocket = static_cast<int>(socket(protoFam, SOCK_STREAM, IPPROTO_TCP));
	else if (_type == UDP)
		_idSocket = static_cast<int>(socket(protoFam, SOCK_DGRAM, 0));
	else {
		_type = NONE;
		std::cout << "Type not recognized." << std::endl;
		success = false;
	}

	return success;
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

