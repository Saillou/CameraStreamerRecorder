#include <iostream>
#include <fstream>
#include <csignal>

#include "Globals/structures.hpp"
#include "Dk/Chronometre.hpp"
#include "Dk/ManagerConnection.hpp"
#include "Dk/VideoStreamWriter.hpp"
#include "Device/DeviceMT.hpp"
#include "Tool/MovieWriter.hpp"

// Globals
bool G_stop = false;


static void signalHandler(int signum) {
	std::cout << "\nInterrupt" << std::endl;
	G_stop = true;
}


// Functions
int main() {	
	// --------------- Interface -------------	
	signal(SIGINT, signalHandler);
	
	// --------------- Devices -------------
	DeviceMT device0("/dev/video0");
	DeviceMT device1("/dev/video1");
	
	device0.open();
	device1.open();
	
	// ---------------- Video Streamer ----------------
	ManagerConnection managerConnection;
	managerConnection.initialize();
	
	Dk::VideoStreamWriter videoStreamer0(managerConnection, 3000);	
	Dk::VideoStreamWriter videoStreamer1(managerConnection, 3001);
	
	videoStreamer0.startBroadcast(device0.getSize(), 3);
	videoStreamer1.startBroadcast(device1.getSize(), 3);
	
	// --------------- Video Recorder -------------
	MovieWriter movieWriter0;
	MovieWriter movieWriter1;
	
	movieWriter0.start("Video0_" + Chronometre::date(), device0.getSize(), device0.getFps());
	movieWriter1.start("Video1_" + Chronometre::date(), device1.getSize(), device1.getFps());
	

	// --------------- Looping -------------
	const int fps(std::max(device0.getFps(), device1.getFps()));
	Chronometre chronoRoutine;
	Gb::Frame frame0;
	Gb::Frame frame1;
	
	while(!G_stop) {
		chronoRoutine.beg();
		
		// - Read device 0
		if(device0.isOpen()) {
			device0.read(frame0);
			
			if(!frame0.empty()) {
				// - TCP
				if(videoStreamer0.isValide()) {
					videoStreamer0.update(frame0);	
				}
					
				// - Recording
				if(movieWriter0.isRecording()) {
					movieWriter0.saveFrame(frame0);	
				}	
			}
		}
		
		// - Read device 1
		if(device1.isOpen()) {
			device1.read(frame1);
			
			if(!frame1.empty()) {
				// - TCP
				if(videoStreamer1.isValide()) {
					videoStreamer1.update(frame1);	
				}
					
				// - Recording
				if(movieWriter1.isRecording()) {
					movieWriter1.saveFrame(frame1);	
				}	
			}
		}
		
		// Wait 'till it's time again		
		chronoRoutine.end();
		Chronometre::wait((int64_t)(1000/fps) - chronoRoutine.ms());
	}

	// --------------- The end -------------
	device0.close();
	movieWriter0.stop();
	videoStreamer0.release();
	
	device1.close();
	movieWriter1.stop();
	videoStreamer1.release();
	
	std::cout << std::endl << "Exit success" << std::endl;
	return 0;
}
