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
#include "Tool/ConfigParser.hpp"


//  --------------------- Variables ------------------------------------
// -- Controle
bool G_stop(false);
ConfigParser::Result G_config;
Chronometre G_chronoRecording;

std::string G_experimentFolder(""); // without (id) in the path ../root/[..](id)/name.ext
std::string G_experimentFolderPath("");
std::vector<char> G_calibrationKey;


// -- Tools
DeviceMT* device0;
DeviceMT* device1;

MovieWriter* movieWriter0;
MovieWriter* movieWriter1;

Dk::VideoStreamWriter* videoStreamer0;	
Dk::VideoStreamWriter* videoStreamer1;


// ------------------------ Functions ----------------------------------
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
	std::cout << " ================================================ " << std::endl;
}
static bool existsOption(int argc, char* argv[], const std::string& option) {
	for(int i = 1; i < argc; i++) {
		if(std::string(argv[i]) == option)
			return true;
	}
	return false;
}

static void writeCalibrationKey(const std::string& path) {
	if(!dirExists(path)) {
		if(!createDir(path)) {
			std::cout << "Failed to created dir for calibration" << std::endl;
			return;
		}
	}
	
	std::ofstream fileCalib;
	fileCalib.open(path.c_str(), std::ios::binary | std::ios::trunc);
	fileCalib.write(G_calibrationKey.data(), G_calibrationKey.size());
	fileCalib.close();	
}
static void onCalibration(int idClient, const Protocole::BinMessage& msg) {
	std::cout << "Calibration received." << std::endl;
	
	if(msg.getSize() > 0)
		G_calibrationKey = msg.getData();
		
	// If record has already started:
	if(dirExists(G_experimentFolderPath)) {
		if(!G_calibrationKey.empty())
			writeCalibrationKey(G_experimentFolderPath+"/Calibration.bin");
	}
	
}

static void onRecordStart() {
	// Search id of folder
	static int id = 0;
	do {
		G_experimentFolderPath = G_experimentFolder + "("+std::to_string(id)+")";
		id++;
	} while(dirExists(G_experimentFolderPath));
	
	
	// Create it
	if(!createDir(G_experimentFolderPath)) {
		std::cout << "Failed to created directory" << std::endl;
		return;
	}
	
	// Start recording and write calibration key (if already computed)
	if(!G_calibrationKey.empty())
		writeCalibrationKey(G_experimentFolderPath+"/Calibration.bin");
		
	if(device0->isOpen() && !movieWriter0->isRecording())
		movieWriter0->start(G_experimentFolderPath+"/Video0", device0->getSize(), device0->getFps());
		
	if(device1->isOpen() && !movieWriter1->isRecording())
		movieWriter1->start(G_experimentFolderPath+"/Video1", device1->getSize(), device1->getFps());
		
	G_chronoRecording.beg();
}
static void onRecordStop() {
	if(movieWriter0->isRecording())
		movieWriter0->stop();
		
	if(movieWriter1->isRecording())
		movieWriter1->stop();
}
static void onStop() {
	G_stop = true;
}



// -------------------------- Signals ----------------------------------
static void signalHandler(int signum) {
	std::cout << "\nInterrupt" << std::endl;
	onStop();
}



// ---------------------------------------------------------------------
// ----------------------------- Main ----------------------------------
// ---------------------------------------------------------------------
int main(int argc, char* argv[]) {
	// -------------------- Options and Configuration ------------------	
	if(existsOption(argc, argv, "-help")) {
		displayHelp();
		return 0;
	}
	bool enableCamera0 = !existsOption(argc, argv, "-noCamera0");
	bool enableCamera1 = !existsOption(argc, argv, "-noCamera1");
	bool enableStream  = !existsOption(argc, argv, "-noStream");
	
	G_config = ConfigParser::parse("../BarnaclesManager/config");
	G_experimentFolder = "../BarnaclesManager/root"+G_config["defaultPath"].toString() + Chronometre::date();
	
	
	// ---------------------- Interface externs ------------------------
	signal(SIGINT, signalHandler);
	
	
		
	// -------------------------- Devices ------------------------------	
	device0 = new DeviceMT("/dev/video0");
	device1 = new DeviceMT("/dev/video1");

	if(enableCamera0)
		device0->open();
		
	if(enableCamera1)
		device1->open();
	
	
	// ------------------------- Video Streamer ------------------------
	ManagerConnection managerConnection;
	managerConnection.initialize();
	
	IpAdress ip0(managerConnection.getGatewayAdress(IpAdress::IP_V6).getTarget(), 3000); 
	IpAdress ip1(managerConnection.getGatewayAdress(IpAdress::IP_V6).getTarget(), 3001); 
	
	videoStreamer0 = new Dk::VideoStreamWriter(managerConnection, ip0, G_config["deviceName"].toString() + " - 0");	
	videoStreamer1 = new Dk::VideoStreamWriter(managerConnection, ip1, G_config["deviceName"].toString() + " - 1");
	
	
	if(device0->isOpen() && enableStream) {
		videoStreamer0->startBroadcast(device0->getSize(), 3);
		videoStreamer0->addCallback(Protocole::BIN_CLBT, onCalibration);
	}
		
	if(device1->isOpen() && enableStream) 
		videoStreamer1->startBroadcast(device1->getSize(), 3);
	
	
	// -------------------------- Video Recorder -----------------------	
	movieWriter0 = new MovieWriter();
	movieWriter1 = new MovieWriter();
		

	// --------------------------- Looping -----------------------------
	const int fps(std::max(device0->getFps(), device1->getFps()));
	Chronometre chronoRoutine;
	Gb::Frame frame0;
	Gb::Frame frame1;
	
	bool isRecording = false;
	if(G_config["recording"].toBool()) {
		onRecordStart();
		isRecording = true;
	}
	
	bool splitRecord = G_config["splitting"].toBool();
	const int64_t NB_MS_MAX = 30*60*1000; // 30mn in milliseconds
	
	while(!G_stop) {		
		chronoRoutine.beg();
		
		// Split recording if necessary
		if(isRecording && splitRecord) {
			if(G_chronoRecording.elapsed_ms() > NB_MS_MAX) {
				onRecordStop();
				onRecordStart();
			}
		}
		
		// - Read device 0
		if(device0->isOpen()) {
			device0->read(frame0);
			
			if(!frame0.empty()) {
				// - TCP
				if(videoStreamer0->isValide())
					videoStreamer0->update(frame0);	
					
				// - Recording
				if(movieWriter0->isRecording())
					movieWriter0->saveFrame(frame0);	
			}
		}
		
		// - Read device 1
		if(device1->isOpen()) {
			device1->read(frame1);
			
			if(!frame1.empty()) {
				// - TCP
				if(videoStreamer1->isValide())
					videoStreamer1->update(frame1);	
					
				// - Recording
				if(movieWriter1->isRecording())
					movieWriter1->saveFrame(frame1);	
			}
		}
		
		// Wait 'till it's time again		
		chronoRoutine.end();
		Chronometre::wait((int64_t)(1000/fps) - chronoRoutine.ms());
	}
	
	

	// -------------------------- The end -------------------------------------
	if(device0->isOpen())
		device0->close();
	if(movieWriter0->isRecording())
		movieWriter0->stop();
	if(videoStreamer0->isValide())
		videoStreamer0->release();
	
	if(device1->isOpen())
		device1->close();
	if(movieWriter1->isRecording())
		movieWriter1->stop();
	if(videoStreamer1->isValide())
		videoStreamer1->release();
		
		
	delete device0;
	delete movieWriter0;
	delete videoStreamer0;
	
	delete device1;
	delete movieWriter1;
	delete videoStreamer1;
	
	
	std::cout << std::endl << "Exit success" << std::endl;
	return 0;
}
