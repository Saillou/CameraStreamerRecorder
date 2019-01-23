#ifndef RECORDER_HPP
#define RECORDER_HPP

#include <iostream>
#include <stdint.h>

extern "C" {
	#include <x264.h>
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/mathematics.h>
	#include <libavformat/avformat.h>
	#include <libavutil/opt.h>
}

#include "../Globals/structures.hpp"

class Recorder {
public:
	// Constructors
	Recorder();
	~Recorder();
	
	// Methods
	bool open(const std::string& fileName, const Gb::Size& size, double fps);
	bool release();
	
	bool write(Gb::Frame& frame);
	
	bool isOpen() const;
	
private:
	// Disable copy
	Recorder& operator=(const Recorder&) = delete;
	Recorder(const Recorder&) = delete;
	
	// Members
	bool _isOpen;
	std::string _fileName;
	Gb::Size _size;
	double _fps;
	
	AVFrame* _inFrame;
	AVCodecContext *_inCc;
	AVCodec* _inCodec;
	
	AVOutputFormat* _fmt;
	AVFormatContext* _fc;
	AVStream* _stream;
	AVCodec* _codec;
	AVDictionary* _opt;
	AVCodecContext* _cc;
	AVPacket _packetOut;
	
	int _gotInput;
	int _gotOutput;
	int _iFrame;
};


#endif
