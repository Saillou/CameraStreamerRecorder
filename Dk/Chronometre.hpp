#ifndef CHRONOMETRE_HPP
#define CHRONOMETRE_HPP

#include <chrono>
#include <time.h>
#include <sstream>
#include <thread>

struct Chronometre {	
public:
	Chronometre():
		_tRef(_now()),
		_tBeg(_tRef),
		_tEnd(_tRef)
	{
	}

	// Methods	
	void beg() {
		_tBeg = _now();
	}
	void end() {
		_tEnd = _now();
	}
	void reset() {
		_tRef = _now();
		_tBeg = _tRef;
		_tEnd = _tRef;
	}
	
	int64_t clock_ms() const {
		return  _diffMs(_tRef, _now());
	}
	int64_t ms() const {
		return _diffMs(_tBeg, _tEnd);
	}
	int64_t elapsed_ms() const {
		return _diffMs(_tBeg, _now());
	}
	
	// Static methods
	static void wait(int ms) {
		if(ms > 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}
	
	static std::string date() {
		time_t rawtime = time(NULL);
		struct tm timeInfo;
		
#ifdef _WIN32
		localtime_s(&timeInfo, &rawtime);
#else
		struct tm *ptrTimeInfo;
		ptrTimeInfo = localtime(&rawtime);
		timeInfo = *ptrTimeInfo;
#endif

		auto __int2paddedStr = [](const int _int, const size_t pad) {
			std::string intstr = std::to_string(_int);
			if(intstr.size() >= pad)
				return intstr;
			else
				return std::string(pad - intstr.size() , '0') + intstr;
		};		
		
		std::stringstream ss;
		ss << __int2paddedStr(timeInfo.tm_year + 1900, 4);
		ss << __int2paddedStr(timeInfo.tm_mon  + 1, 2);
		ss << __int2paddedStr(timeInfo.tm_mday,	2);
		ss << "_";
		ss << __int2paddedStr(timeInfo.tm_hour,	2);
		ss << __int2paddedStr(timeInfo.tm_min,	2);
		ss << __int2paddedStr(timeInfo.tm_sec, 	2);
		
		return ss.str();
	}
	
private:
	typedef std::chrono::high_resolution_clock::time_point 	_timePoint;
	
	// Static methods
	static _timePoint _now() {
		return std::chrono::high_resolution_clock::now();
	}
	static int64_t _diffMs(const _timePoint& tA, const _timePoint& tB) {
		return std::chrono::duration_cast<std::chrono::duration<int64_t, std::milli>>(tB - tA).count();
	}
	
	// Members
	_timePoint _tRef; // Memorise time of the chrono's birth / last reset
	_timePoint _tBeg;
	_timePoint _tEnd;
};

#endif
