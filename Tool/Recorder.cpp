#include "Recorder.hpp"

// Constructors
Recorder::Recorder() :
	_isOpen(false),
	_fileName(""),
	_size(0,0),
	_fps(0),
	_inFrame(nullptr),
	_inCc(nullptr),
	_inCodec(nullptr),
	_fmt(nullptr),
	_fc(nullptr),
	_stream(nullptr),
	_codec(nullptr),
	_opt(nullptr),
	_cc(nullptr),
	_gotInput(0),
	_gotOutput(0),
	_iFrame(0)
{
	av_register_all();
	avcodec_register_all();
}

Recorder::~Recorder() {
	release();
}


// Methods
bool Recorder::open(const std::string& fileName, const Gb::Size& size, double fps) {
	if(_isOpen)
		release();
	
	int resAction = -1;
	
	_fileName 	= fileName + ".mp4";
	_size 		= size;
	_fps 		= fps;
	
	// - Output
	// Stream
	resAction = avformat_alloc_output_context2(&_fc, NULL, NULL, _fileName.c_str());
	if(resAction < 0) {
		std::cout << "Couldn't alloc output contex [error:" << resAction << "]" << std::endl;
		return false;
	}
	
	_stream = avformat_new_stream(_fc, 0);
	_codec 	= avcodec_find_encoder_by_name("libx264");
	_fmt 	= av_guess_format("mp4", NULL, NULL);
	
	// Codec
	av_dict_set(&_opt, "preset", "ultrafast", 0); // Speed over size
	av_dict_set(&_opt, "crf", "25", 0); // Quality 0(best) - 51(worst)
	
	_cc 			= avcodec_alloc_context3(_codec);
	_cc->width 		= _size.width;
	_cc->height 	= _size.height;
	_cc->pix_fmt	= AV_PIX_FMT_YUV420P;
	_cc->time_base 	= (AVRational){1, (int)_fps};
	_cc->framerate 	= (AVRational){(int)_fps, 1};
	_cc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			
	resAction = avcodec_open2(_cc, _codec, &_opt);
	if(resAction < 0) {
		std::cout << "Couldn't open codec [error:" << resAction << "]" << std::endl;
		return false;
	}
	
	_stream->codec = _cc;
	
	resAction = avio_open(&_fc->pb, _fileName.c_str(), AVIO_FLAG_WRITE);
	if(resAction < 0) {
		std::cout << "Couldn't open format context [error:" << resAction << "]" << std::endl;
		return false;
	}
	
	avformat_write_header(_fc, &_opt);
	av_dict_free(&_opt);
	
	// - Input
	// Prepare decoding	
	_inCodec 	= avcodec_find_decoder(AV_CODEC_ID_MJPEG);
	_inCc 		= avcodec_alloc_context3(_inCodec);
	avcodec_open2(_inCc, _inCodec, 0);
	
	_inFrame 			= av_frame_alloc();
	_inFrame->width 	= size.width;
	_inFrame->height 	= size.height;
	_inFrame->format 	= AV_PIX_FMT_YUV420P;	
	
	resAction = av_frame_get_buffer(_inFrame, 1);
	if(resAction < 0) {
		std::cout << "Couldn't alloc frame memory [error:" << resAction << "]" << std::endl;
		return false;
	}
	
	_isOpen = true;
	
	return _isOpen;
}
bool Recorder::release() {
	if(!_isOpen)
		return false;
	
	// delayed frames
	while(_gotOutput) {
		avcodec_encode_video2(_cc, &_packetOut, NULL, &_gotOutput);
		
		if(_gotOutput) {
			fflush(stdout);
			av_packet_rescale_ts(&_packetOut, _cc->time_base, _stream->time_base);
			_packetOut.stream_index = _stream->index;
			av_interleaved_write_frame(_fc, &_packetOut);
			av_packet_unref(&_packetOut);	
		}
	}
	
	// Finish it
	av_write_trailer(_fc);
	if(!(_fmt->flags & AVFMT_NOFILE))
		avio_closep(&_fc->pb);
		
	av_frame_free(&_inFrame);
	avcodec_close(_stream->codec);
	
	_isOpen = false;
	_iFrame = 0;
	_gotInput = 0;
	_gotOutput = 0;
	
	return true;
}

bool Recorder::write(Gb::Frame& frame) {
	if(!_isOpen)
		return false;
		
	int resAction = -1;
	
	// Read frame
	AVPacket inPacket;
	av_init_packet(&inPacket);
	
	inPacket.data = &frame.buffer[0];
	inPacket.size = (int)frame.length();
	
	resAction = avcodec_decode_video2(_inCc, _inFrame, &_gotInput, &inPacket);
	if(resAction < 0) {
		std::cout << "Couldn't decode packet [error:" << resAction << "]" << std::endl;
		return false;
	}
	
	// Write
	av_init_packet(&_packetOut);
	_packetOut.data 	 = NULL;
	_packetOut.size 	 = 0;
	_inFrame->pts 		 = _iFrame;
	
	resAction = avcodec_encode_video2(_cc, &_packetOut, _inFrame, &_gotOutput);
	if(resAction < 0) {
		std::cout << "Couldn't encode frame [error:" << resAction << "]" << std::endl;
		return false;
	}
	
	if(_gotOutput) {
		fflush(stdout);
		av_packet_rescale_ts(&_packetOut, _cc->time_base, _stream->time_base);
		_packetOut.stream_index = _stream->index;
		av_interleaved_write_frame(_fc, &_packetOut);
		av_packet_unref(&_packetOut);
	}
	
	_iFrame++;
	return true;
}

bool Recorder::isOpen() const {
	return _isOpen;
}

