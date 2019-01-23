#ifndef DEIVCE_HPP
#define DEVICE_HPP

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <string>
#include <linux/videodev2.h>

#include "../Globals/structures.hpp"

class Device {
public:
	// Constructors
	explicit Device(const std::string& pathVideo);
	~Device();
	
	// Methods
	bool open();
	bool close();
	
	bool grab();
	bool retrieve(Gb::Frame& frame);
	bool read(Gb::Frame& frame);
	
	const Gb::Size getSize() const;
	
private:
	// Structures
	struct FrameBuffer {
		void *start;
		size_t length;
	};
	struct FrameFormat {
		int width;
		int height;
		int format;	
	};
	
	// Statics
	static unsigned char _clip(int value);
	static int _xioctl(int fd, int request, void *arg);
	
	// Methods
	bool _initDevice();
	bool _initMmap();
	bool _askFrame();
	
	bool _treat(Gb::Frame& frame);
	void _perror(const std::string& message) const;
	
	// Members
	int _fd;
	std::string _path;
	FrameFormat	_format;
	FrameBuffer _buffer;
	Gb::Frame 	_rawData;
};
#endif
