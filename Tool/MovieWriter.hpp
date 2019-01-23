#ifndef MOVIE_WRITER_HPP
#define MOVIE_WRITER_HPP

#include "../Dk/Chronometre.hpp"
#include "../Globals/structures.hpp"
#include "Recorder.hpp"

#include <iostream>
#include <atomic>
#include <mutex>
#include <thread>

class MovieWriter {
	
public:
	MovieWriter();
	~MovieWriter();
	
	// Methods
	bool start(const std::string& name, const Gb::Size& size, double fps);
	void stop();
	
	void saveFrame(const Gb::Frame& frame);
	
	// Getters
	bool isRecording() const;
	const std::string& getName() const;
	
private:
	// Disable copy
	MovieWriter& operator=(const MovieWriter &) = delete;
	MovieWriter(const MovieWriter&) = delete;

	// Members
	std::atomic<bool> 	_isRecording;
	std::atomic<double> _fpsRate;
	
	Recorder 		_recorder;
	Gb::Frame 		_frameBuffed;
	
	std::string 	_fileName;
	std::thread*	_pThread;
	std::mutex 		_mutFrame;
	
	// Methods
	void _saveFrame();
};

#endif 
