#include "Device.hpp"

// Constructors
Device::Device(const std::string& pathVideo):
	_fd(-1), 
	_path(pathVideo), 
	_format({0, 0, 0}),
	_buffer({(void*)nullptr, (size_t)0})
{
	// Nothing else to do
}
Device::~Device() {
	if(_fd != -1)
		close();
}

// Public Methods
bool Device::open() {
	_fd = ::open(_path.c_str(), O_RDWR | O_NONBLOCK, 0);
	
	if(_fd == -1 || !_initDevice() || !_initMmap()) {
		_perror("Opening device");
		if(_fd != -1) 
			::close(_fd);
			
		return false;
	}
	
	return true;
}
bool Device::close() {
	// Stop capture
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == _xioctl(_fd, VIDIOC_STREAMOFF, &type)) {
        _perror("Stop Capture");
        return false;
    }
    
	if(-1 == munmap(_buffer.start, _buffer.length)) {
        _perror("Memory unmap");
        return false;
    }
    
	if(_fd != -1) {
		::close(_fd);
		_fd = -1;
	}
	return true;
}

bool Device::grab() {
	struct v4l2_buffer buf = {0};
	buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory 	= V4L2_MEMORY_MMAP;
			
	for(;;) {
		// Wait event on fd
		struct pollfd fdp;
		fdp.fd 			= _fd;
		fdp.events 		= POLLIN | POLLOUT; // inputs
		fdp.revents		= 0; // outputs
		
		int r = poll(&fdp, 1, 1000);
		
		// Error ?
		if(r < 1) {
			if(-1 == r) {
				if(EINTR == errno) {// Interrupted 
					printf(".\n");
					continue;
				}
				_perror("Waiting for Frame");
			}
			if(0 == r) {				
				_perror("Timeout"); 
			}
			return false;
	 	}
	
		// Grab frame
		if(-1 == _xioctl(_fd, VIDIOC_DQBUF, &buf)) {
			if(EAGAIN == errno)
				continue;
				
		    _perror("Grab Frame");
		    return false;
		}
		
		// Check size
		_buffer.length = (buf.bytesused > 0) ? buf.bytesused : _buffer.length;	
		return true;
	}
	return false;
}
bool Device::retrieve(Gb::Frame& frame) {
	_rawData = Gb::Frame(
		reinterpret_cast<unsigned char*>(_buffer.start), 
		static_cast<unsigned long>(_buffer.length),
		Gb::Size(_format.width, _format.height)
	).clone();
	
	_askFrame();
    	
	return _treat(frame);
}
bool Device::read(Gb::Frame& frame) { 
	return (grab() && retrieve(frame));
}

const Gb::Size Device::getSize() const {
	return Gb::Size(_format.width, _format.height);
}

// Statics Private Methods
unsigned char Device::_clip(int value) {
	return (unsigned char)((unsigned)value <= 255 ? value : value > 0 ? 255 : 0);
}
int Device::_xioctl(int fd, int request, void *arg) {
        int r(-1);
        do {
        	r = ioctl (fd, request, arg);
        } while (-1 == r && EINTR == errno);
        return r;
}

// Private methods
bool Device::_initDevice() {
	// Capabilities : v4l2, capture, streaming
    struct v4l2_capability cap = {0};
    if (-1 == _xioctl(_fd, VIDIOC_QUERYCAP, &cap)) {
        _perror("Querying Capabilities");
        return false;
    }
 
    printf( "Driver Caps:\n"
        "  Driver: \"%s\"\n"
        "  Card: \"%s\"\n"
        "  Bus: \"%s\"\n"
        "  Version: %d.%d\n"
        "  Capabilities: %08x\n",
        cap.driver,
        cap.card,
        cap.bus_info,
        (cap.version>>16)&&0xff,
        (cap.version>>24)&&0xff,
        cap.capabilities);
 
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		_perror("Video capture device");
		return false;
	}
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		_perror("Video capture streaming");
		return false;
	}
 
	// Cropping 
    struct v4l2_cropcap cropcap = {0};
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == _xioctl (_fd, VIDIOC_CROPCAP, &cropcap)) {
        _perror("Querying Cropping Capabilities");
        return false;
    }
 
    printf( "Camera Cropping:\n"
            "  Bounds: %dx%d+%d+%d\n"
            "  Default: %dx%d+%d+%d\n"
            "  Aspect: %d/%d\n",
            cropcap.bounds.width, cropcap.bounds.height, cropcap.bounds.left, cropcap.bounds.top,
            cropcap.defrect.width, cropcap.defrect.height, cropcap.defrect.left, cropcap.defrect.top,
            cropcap.pixelaspect.numerator, cropcap.pixelaspect.denominator);
 
    int support_grbg10 = 0;

    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    char fourcc[5] = {0};
    char c, e;
    printf("  FMT : CE Desc\n--------------------\n");
    while (0 == _xioctl(_fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
        strncpy(fourcc, (char *)&fmtdesc.pixelformat, 4);
        if (fmtdesc.pixelformat == V4L2_PIX_FMT_SGRBG10)
            support_grbg10 = 1;
            
        c = fmtdesc.flags & 1? 'C' : ' ';
        e = fmtdesc.flags & 2? 'E' : ' ';
        printf("  %s: %c%c %s\n", fourcc, c, e, fmtdesc.description);
        fmtdesc.index++;
	}
	
    struct v4l2_format fmt = {0};
    fmt.type 				= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width 		= 640;
    fmt.fmt.pix.height 		= 480;
    //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // Doesn't work for 2 cameras 640*480. (320*200 is ok)
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field 		= V4L2_FIELD_ANY;
    
    _format.width  = fmt.fmt.pix.width;
    _format.height = fmt.fmt.pix.height;
    _format.format = fmt.fmt.pix.pixelformat;
    
    if (-1 == _xioctl(_fd, VIDIOC_S_FMT, &fmt)) {
        _perror("Setting Pixel Format");
        return false;
    }
 
    strncpy(fourcc, (char *)&fmt.fmt.pix.pixelformat, 4);
    printf( "Selected Camera Mode:\n"
            "  Width: %d\n"
            "  Height: %d\n"
            "  PixFmt: %s\n"
            "  Field: %d\n",
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
            fourcc,
            fmt.fmt.pix.field);
            
	return true;
}
 
bool Device::_initMmap() {
	// Init buffers
    struct v4l2_requestbuffers req = {0};
    req.count 	= 1;
    req.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory 	= V4L2_MEMORY_MMAP;
 
    if (-1 == _xioctl(_fd, VIDIOC_REQBUFS, &req)) {
        _perror("Requesting Buffer");
        return false;
    }
 
    struct v4l2_buffer buf = {0};
    buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory 	= V4L2_MEMORY_MMAP;
    buf.index 	= 0;
    
    if(-1 == _xioctl(_fd, VIDIOC_QUERYBUF, &buf)) {
        _perror("Querying Buffer");
        return false;
    }
 
 	// Memory map
    _buffer.start 	= mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, buf.m.offset);
    _buffer.length 	= buf.bytesused > 0 ? buf.bytesused : buf.length;
    if(_buffer.start == MAP_FAILED) {
        _perror("Mapping");
        return false;    
    }
    printf("Length: %d\n", _buffer.length);
    
    
	// Start capture
    if(!_askFrame())
    	return false;
 
    if(-1 == _xioctl(_fd, VIDIOC_STREAMON, &buf.type)) {
        _perror("Start Capture");
        return false;
    }
    
    return true;
}
bool Device::_askFrame() {
	struct v4l2_buffer buf = {0};
	buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory 	= V4L2_MEMORY_MMAP;
	
    if(-1 == _xioctl(_fd, VIDIOC_QBUF, &buf)) {
        _perror("Query Buffer");
        return false;
    }
    return true;
}

bool Device::_treat(Gb::Frame& frame) {
	frame = _rawData.clone();
	return !frame.empty();
}
void Device::_perror(const std::string& message) const {
	std::string mes = " [" + _path + ", " + std::to_string(_fd) + "] " + message;
	perror(mes.c_str());
} 

