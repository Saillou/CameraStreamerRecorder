#include <iostream>
#include <fstream>
#include <csignal>

#include <sys/types.h>
#include <sys/stat.h>

#include "Globals/structures.hpp"
#include "Dk/Chronometre.hpp"
#include "Dk/ManagerConnection.hpp"
#include "Dk/VideoStreamWriter.hpp"
#include "Device/DeviceMT.hpp"
#include "Tool/MovieWriter.hpp"

// Globals
bool G_stop = false;
std::string G_experimentFolder = "";

static void signalHandler(int signum) {
	std::cout << "\nInterrupt" << std::endl;
	G_stop = true;
}

static bool dirExists(const std::string& path) {
	struct stat info;
	
	if(stat(path.c_str(), &info) != 0)
		return false;
	
	if(info.st_mode & S_IFDIR)
		return true;
		
	return false;
}
static bool createDir(const std::string& path) {
	return mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1;
}

static void displayHelp () {
	std::cout << " ==================== H E L P =================== " << std::endl;
	std::cout << "Available options: " << std::endl;
	std::cout << "\t -noCamera0 \t// Disable camera 0." << std::endl;   
	std::cout << "\t -noCamera1 \t// Disable camera 1." << std::endl;   
	std::cout << "\t -noStream  \t// Disable streaming." << std::endl;   
	std::cout << "\t -noRecord  \t// Disable recording." << std::endl;   
	std::cout << " ================================================ " << std::endl;
}
static bool existsOption(int argc, char* argv[], const std::string& option) {
	for(int i = 1; i < argc; i++) {
		if(std::string(argv[i]) == option)
			return true;
	}
	return false;
}

static void onCalibration(int idClient, const Protocole::BinMessage& msg) {
	std::cout << "Calibration received." << std::endl;
	if(msg.getSize() > 0) {
		if(!dirExists(G_experimentFolder)) {
			if(!createDir(G_experimentFolder)) {
				std::cout << "Failed to created dir for calibration" << std::endl;
				return;
			}
		}
		
		std::ofstream fileCalib;
		fileCalib.open(G_experimentFolder+"/Calibration.bin", std::ios::binary | std::ios::trunc);
		fileCalib.write(msg.getData().data(), msg.getSize());
		fileCalib.close();
	}
}


// Functions
int main(int argc, char* argv[]) {	
	// --------------- Options -------------	
	if(existsOption(argc, argv, "-help")) {
		displayHelp();
		return 0;
	}
	
	bool enableCamera0 = !existsOption(argc, argv, "-noCamera0");
	bool enableCamera1 = !existsOption(argc, argv, "-noCamera1");
	bool enableStream  = !existsOption(argc, argv, "-noStream");
	bool enableRecord  = !existsOption(argc, argv, "-noRecord");
	
	// --------------- Interface -------------	
	signal(SIGINT, signalHandler);
	G_experimentFolder = "../BarnaclesManager/root/"+Chronometre::date();
	
	if(!dirExists(G_experimentFolder)) 
		if(!createDir(G_experimentFolder)) 
			std::cout << "Failed to created directory" << std::endl;
		
	// --------------- Devices -------------
	DeviceMT device0("/dev/video0");
	DeviceMT device1("/dev/video1");
	
	if(enableCamera0)
		device0.open();
		
	if(enableCamera1)
		device1.open();
	
	// ---------------- Video Streamer ----------------
	ManagerConnection managerConnection;
	managerConnection.initialize();
	
	IpAdress ip0(managerConnection.getGatewayAdress(IpAdress::IP_V6)); 
	IpAdress ip1(managerConnection.getGatewayAdress(IpAdress::IP_V6)); 
	
	ip0.setPort(3000);
	ip1.setPort(3001);
	
	Dk::VideoStreamWriter videoStreamer0(managerConnection, ip0, "WiFi Device - 0");	
	Dk::VideoStreamWriter videoStreamer1(managerConnection, ip1, "WiFi Device - 1");
	
	
	if(device0.isOpen() && enableStream) {
		videoStreamer0.startBroadcast(device0.getSize(), 3);
		videoStreamer0.addCallback(Protocole::BIN_CLBT, onCalibration);
	}
		
	if(device1.isOpen() && enableStream) 
		videoStreamer1.startBroadcast(device1.getSize(), 3);
		
	
	
	// --------------- Video Recorder -------------
	MovieWriter movieWriter0;
	MovieWriter movieWriter1;
	
	if(device0.isOpen() && enableRecord)
		movieWriter0.start(G_experimentFolder+"/Video0", device0.getSize(), device0.getFps());
		
	if(device1.isOpen() && enableRecord)
		movieWriter1.start(G_experimentFolder+"/Video1", device1.getSize(), device1.getFps());
	

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
				if(videoStreamer0.isValide())
					videoStreamer0.update(frame0);	
					
				// - Recording
				if(movieWriter0.isRecording())
					movieWriter0.saveFrame(frame0);	
			}
		}
		
		// - Read device 1
		if(device1.isOpen()) {
			device1.read(frame1);
			
			if(!frame1.empty()) {
				// - TCP
				if(videoStreamer1.isValide())
					videoStreamer1.update(frame1);	
					
				// - Recording
				if(movieWriter1.isRecording())
					movieWriter1.saveFrame(frame1);	
			}
		}
		
		// Wait 'till it's time again		
		chronoRoutine.end();
		Chronometre::wait((int64_t)(1000/fps) - chronoRoutine.ms());
	}

	// --------------- The end -------------
	if(device0.isOpen())
		device0.close();
	if(movieWriter0.isRecording())
		movieWriter0.stop();
	if(videoStreamer0.isValide())
		videoStreamer0.release();
	
	if(device1.isOpen())
		device1.close();
	if(movieWriter1.isRecording())
		movieWriter1.stop();
	if(videoStreamer1.isValide())
		videoStreamer1.release();
	
	std::cout << std::endl << "Exit success" << std::endl;
	return 0;
}
