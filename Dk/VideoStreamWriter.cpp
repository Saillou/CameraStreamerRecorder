#include "VideoStreamWriter.hpp"

using namespace Dk;
using namespace Protocole;

// Constructors
VideoStreamWriter::VideoStreamWriter(ManagerConnection& managerConnection, const IpAdress& ipGateway, const std::string& name) :
	_server(managerConnection.createServer(Socket::TCP, Socket::BLOCKING, ipGateway)),
	_valide(false),
	_threadClients(nullptr),
	_format(/*Height*/0, /*Width*/0, /*Channels*/0),
	_frameToSend()
{
	_format.name = name;
	_valide = _server != nullptr;
}
VideoStreamWriter::~VideoStreamWriter() {
	release();
	
	if(_threadClients != nullptr)
		delete _threadClients;
		
	_pCallbacks.clear();
}

// Methods
void VideoStreamWriter::handleClients() {
	std::cout << "Wait for clients" << std::endl;
	
	while(_atomRunning) {	
		int idClient = _server->nextClient();
		
		// Wait a new client
		if(idClient <= 0)
			idClient = _server->waitClient(350);
		

		if(idClient > 0) {
			_handleClient(idClient);
		}	
		
		// Update state		
		if(_atomRunning && idClient > 0)
			std::cout << "Wait for clients" << std::endl;
	} 
}

void VideoStreamWriter::_handleClient(int idClient) {
	std::cout << std::endl << "Handle new client " << idClient << std::endl;
	
	// -- Handle client --
	bool clientWantToQuit = false;

	do {		
		// Deals with the socket
		BinMessage msg;
		if(!_server->read(msg, idClient)) {
			if(errno == EWOULDBLOCK) { // timesup
				errno = 0;
				if(_server->waitClient(350) > 0) { // new client !
					break;
				}
			}
			else { // error
				break;
			}
		}
		
		// Treat client
		_invokeCallbacks(idClient, msg);
		clientWantToQuit = _treatClient(idClient, msg);
	} while(!clientWantToQuit && _atomRunning);
	
	_server->closeSocket(idClient);
	std::cout << "Client disconnected. (" << clientWantToQuit << ")" << std::endl;	
}

bool VideoStreamWriter::_treatClient(const int idClient, const BinMessage& msgIn) {
	BinMessage msgOut;
	bool clientWantToQuit = false;
	
	switch(msgIn.getAction()) {
		// Client quit
		case BIN_QUIT:
			clientWantToQuit = true;
		break;
		
		// Frame info asked - change requested
		case BIN_INFO:	
		{
			// Changes are requested
			if(msgIn.getSize() > 0) 
				_changeFormat(FormatStream(CmdMessage(Message::To_string(msgIn.getData())))); // clojure style lol
			
			// Send format
			msgOut.set(BIN_MCMD, Message::To_string(_format.toCmd().serialize()));
			_server->write(msgOut, idClient);
		}
		break;
		
		// Send a frame
		case BIN_GAZO: 
			_mutexFrame.lock();
				msgOut.set(BIN_GAZO, (size_t)_frameToSend.length(), (const char*)_frameToSend.start());
			_mutexFrame.unlock();
			
			// Write the message						
			_server->write(msgOut, idClient);
		break;
	}
	
	
	return clientWantToQuit;
}
void VideoStreamWriter::_invokeCallbacks(int idClient, const Protocole::BinMessage& msgIn) {
	// Check callbacks
	auto callback = _pCallbacks.find(msgIn.getAction());
	
	if(callback != _pCallbacks.end())
		callback->second(idClient, msgIn);	
}

const Protocole::FormatStream& VideoStreamWriter::startBroadcast(const Gb::Size& size, int channels) {	
	// Not working
	if(!_valide)
		return _format; // Empty, can be evaluated to false
	
	// Read camera format
	_format.width 	 	= size.width;
	_format.height 	 	= size.height;
	_format.channels 	= 3;
	_format.fps 		= 30;
	
	// Init Thread 
	_atomRunning = true;
	
	if(_threadClients == nullptr)
		_threadClients = new std::thread(&VideoStreamWriter::handleClients, this);	
	
	return _format;
}

bool VideoStreamWriter::_changeFormat(const Protocole::FormatStream& format) {
	// Not working
	if(!_valide)
		return false;
		
	// Cannot change frame parameters
	if(format.height != _format.height || format.width != _format.width || format.channels != _format.channels)
		return false;
	
	// Update modifications
	//   ...
	
	return true;
}

bool VideoStreamWriter::update(const Gb::Frame& newFrame) {
	if(!_valide)
		return false;
	
	// Should be same format as the inital one
	if(newFrame.size.width != _format.width || newFrame.size.height != _format.height)
		return false;
	
	// Everything ok ? Send to transfert pipe
	_mutexFrame.lock();
		_frameToSend = newFrame.clone();
	_mutexFrame.unlock();
	
	_atomImageUpdated = true;	
	
	return true;
}

void VideoStreamWriter::release() {
	_atomRunning = false;	
	_valide = false;
	
	if(_threadClients != nullptr)
		if(_threadClients->joinable())
			_threadClients->join();		
}

bool VideoStreamWriter::addCallback(const size_t BIN_CODE, std::function<void(int, const Protocole::BinMessage&)> f) {
	_pCallbacks[BIN_CODE] = f;
	return true;
}
bool VideoStreamWriter::removeCallback(const size_t BIN_CODE) {
	_pCallbacks.erase(BIN_CODE);
	return true;
}

// Getters
bool VideoStreamWriter::isValide() const {
	return _valide;
}
