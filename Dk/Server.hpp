#ifndef SERVER_H
#define SERVER_H

#include "Socket.hpp"

#include <memory>
#include <thread>
#include <atomic>

class Server : public Socket {
public:
	// Constructors
	Server(const IpAdress& ipGateway = IpAdress::localhost(IpAdress::IP_V4), const int maxPending = 10);
	~Server();
	
	// Methods
	bool initialize(const CONNECTION_TYPE type, const CONNECTION_MODE mode) override;
	
	int waitClient(long ms = -1);
	int nextClient() const;
	
	void closeSocket(int& idSocket);
	void closeAll();
	
	
protected:
	// Methods
	
	// Members
	int _maxPending;
	std::vector<int> _idScketsConnected;
	
// -------- Nested helper --------
public:
	class ThreadWrite {
	public:
		// Constructors
		ThreadWrite(std::shared_ptr<Server> server, const Protocole::BinMessage& msg, const int idClient);
		~ThreadWrite();
		
		// Methods
		void write(std::shared_ptr<Server> server, const Protocole::BinMessage& msg, const int idClient);
		bool isActive();

	private: 
		// Members
		std::atomic<bool> _atomActive{true};
		std::thread* _ptrThread;
	};
};

#endif
