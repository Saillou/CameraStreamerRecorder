#include "DeviceMT.hpp"

// Constructors
DeviceMT::DeviceMT(const std::string& path) :
	_path(path),
	_device(path),
	_frameIntern(),
	_running(false),
	_flagUpdate(false),
	_pThreadLoop(nullptr),
	_fps(30)
{
	// Nothing else : wait for open
}
DeviceMT::~DeviceMT() {
	if(_running)
		close();
}

// Public methods - Main thread
bool DeviceMT::open() {
	if(_device.open()) {
		_running = true;
		_pThreadLoop = new std::thread(&DeviceMT::_loop, this);
	}
	return _running;
}
bool DeviceMT::close() {
	_running 	= false;
	_flagUpdate = false;
	
	if(_pThreadLoop) {
		if(_pThreadLoop->joinable())
			_pThreadLoop->join();
		delete _pThreadLoop;
	}
	_pThreadLoop = nullptr;
	
	_device.close();
	
	return !_running;
}
bool DeviceMT::read(Gb::Frame& frameDst) {
	// Update the accessible frame
	bool updated = false;
	
	if(_flagUpdate) {
		_mutFrame.lock();
			frameDst = _frameIntern.clone();
		_mutFrame.unlock();
		
		_flagUpdate = false;
		updated = true;
	}

	return updated;
}
bool DeviceMT::isOpen() const {
	return _running;
}

const Gb::Size DeviceMT::getSize() const {
	return _device.getSize();
}
const int DeviceMT::getFps() const {
	return _fps;
}

// Private methods - Threaded
void DeviceMT::_loop() {
	Gb::Frame frameDevice;
	Chronometre chronoRoutine;
	
	while(_running) {
		chronoRoutine.beg();
		
		_device.read(frameDevice);		
		if(frameDevice.empty())
			continue;	
			
		_mutFrame.lock();
			_frameIntern = frameDevice.clone();
		_mutFrame.unlock();
		_flagUpdate = true;
		
		chronoRoutine.end();
		Chronometre::wait((int64_t)(1000/_fps) - chronoRoutine.ms());
	}
}




