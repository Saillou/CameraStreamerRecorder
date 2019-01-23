#include "MovieWriter.hpp"

MovieWriter::MovieWriter() :
	_isRecording(false),
	_fpsRate(0),
	_recorder(),
	_frameBuffed(),
	_fileName(""),
	_pThread(nullptr)
{
	// wait for start
}

MovieWriter::~MovieWriter() {	
	stop();
}

// Public methods
bool MovieWriter::start(const std::string& fileName, const Gb::Size& sizeFrame, double fps) {
	if(_isRecording || fps <= 0 || sizeFrame.area() <= 0)
		return false;
		
	_fileName = fileName;
	_fpsRate  = fps;
	
	_recorder.open(fileName, sizeFrame, fps);
	_isRecording = true;
	
	_pThread = new std::thread(&MovieWriter::_saveFrame, this);
}
void MovieWriter::stop() {
	if(_isRecording) {
		// Reset members
		_isRecording 	= false;
		_fileName 		= "";
		_fpsRate		= 0;
		
		_mutFrame.lock();
			_frameBuffed.clear();
		_mutFrame.unlock();
		
		// Terminate thread
		if(_pThread) {
			if(_pThread->joinable()) {
				_pThread->join();
			}
			
			delete _pThread;
		}	
		
		// Finish recording
		_recorder.release();
	}
}

void MovieWriter::saveFrame(const Gb::Frame& frame) {
	if(!_isRecording || frame.empty()) 
		return;
		
	// Acquire frame
	_mutFrame.lock();
		_frameBuffed = frame.clone();
	_mutFrame.unlock();
}

// Getters
bool MovieWriter::isRecording() const {
	return _isRecording;
}
const std::string& MovieWriter::getName() const {
	return _fileName;
}

// Private methods
void MovieWriter::_saveFrame() {
	Chronometre chronoRoutine;
	
	while(_isRecording) {
		chronoRoutine.beg();
		
		// --- Not ready yet ---
		_mutFrame.lock();
			bool ready = !_frameBuffed.empty() && _recorder.isOpen();
		_mutFrame.unlock();
		
		if(!ready) {
			Chronometre::wait(50);
			continue;
		}
		
		// --- Write frame  ---	
		_mutFrame.lock();
			Gb::Frame frame = _frameBuffed.clone();
		_mutFrame.unlock();
		
		// Write frame in the movie [Can take time, so we locked the mutex before]
		_recorder.write(frame);
		
		
		// --- Wait 'till it's time again ---		
		chronoRoutine.end();
		Chronometre::wait((int64_t)(1000/_fpsRate) - chronoRoutine.elapsed_ms());
	}
}
