#ifndef DEVICE_MT
#define DEVICE_MT

#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

#include "Device.hpp"
#include "../Globals/structures.hpp"
#include "../Dk/Chronometre.hpp"

class DeviceMT {
public:
	// Constructors
	explicit DeviceMT(const std::string& path);
	~DeviceMT();
	
	// Methods
	bool open();
	bool close();
	bool read(Gb::Frame& frameDst);
	bool isOpen() const;
	
	const Gb::Size getSize() const;
	const int getFps() const;
	
private:
	// Methods
	void _loop();
	
	// Members
	std::string _path;
	Device 		_device;
	Gb::Frame 	_frameIntern;
	int			_fps;
	
	std::atomic<bool> 	_running;
	std::atomic<bool> 	_flagUpdate;
	std::thread* 		_pThreadLoop;
	std::mutex 			_mutFrame;
};

#endif
