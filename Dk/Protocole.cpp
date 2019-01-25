#include "Protocole.hpp"

using namespace Protocole;

// -------------------- FormatStream --------------------
FormatStream::FormatStream(int h, int w, int c) : 
	height(h),
	width(w),
	channels(c),
	fps(0),
	hue(-1),
	saturation(-1),
	brightness(-1),
	contrast(-1),
	exposure(-1),
	autoExposure(0),
	name("Device")
{	
	/* Nothing else to do. */	
}
FormatStream::FormatStream(const CmdMessage& cmd) {
	fromCmd(cmd);
}

// Methods
bool FormatStream::isEmpty() const {
	return height*width*channels == 0;
}

void FormatStream::fromCmd(const CmdMessage& cmd) {
	width 	 		= static_cast<int>(Message::To_unsignedInt(cmd.getCommand(CMD_WIDTH).second));
	height 	 		= static_cast<int>(Message::To_unsignedInt(cmd.getCommand(CMD_HEIGHT).second));
	channels 		= static_cast<int>(Message::To_unsignedInt(cmd.getCommand(CMD_CHANNEL).second));
	fps 			= static_cast<int>(Message::To_unsignedInt(cmd.getCommand(CMD_FPS).second));
	hue 			= Message::To_double(cmd.getCommand(CMD_HUE).second);
	saturation 		= Message::To_double(cmd.getCommand(CMD_SATURATION).second);
	brightness		= Message::To_double(cmd.getCommand(CMD_BRIGHTNESS).second);
	contrast 		= Message::To_double(cmd.getCommand(CMD_CONTRAST).second);
	exposure 		= Message::To_double(cmd.getCommand(CMD_EXPOSURE).second);	
	autoExposure	= Message::To_double(cmd.getCommand(CMD_AUTO_EXPOSURE).second);	
	name			= cmd.getCommand(CMD_NAME).second;
}
const CmdMessage FormatStream::toCmd() const {
	CmdMessage cmd;
	
	cmd.addCommand(CMD_HEIGHT, 			std::to_string(height));
	cmd.addCommand(CMD_WIDTH, 			std::to_string(width));
	cmd.addCommand(CMD_CHANNEL,			std::to_string(channels));
	cmd.addCommand(CMD_FPS,				std::to_string(fps));
	cmd.addCommand(CMD_HUE,				std::to_string(hue));
	cmd.addCommand(CMD_SATURATION,		std::to_string(saturation));
	cmd.addCommand(CMD_BRIGHTNESS,		std::to_string(brightness));
	cmd.addCommand(CMD_CONTRAST,		std::to_string(contrast));
	cmd.addCommand(CMD_EXPOSURE,		std::to_string(exposure));
	cmd.addCommand(CMD_AUTO_EXPOSURE,	std::to_string(autoExposure));
	cmd.addCommand(CMD_NAME,			name);
	
	return cmd;
}

// -------------------- Base message --------------------
Message::Message() {
	// Is there something to do?
}
Message::~Message() {
	// Is there something to do?
}
		
// Statics
size_t Message::To_unsignedInt(const std::string& digits, const int base) {
	size_t res = 0;
	for(auto it = digits.begin(); it != digits.end(); ++it) {
		res = (int)(*it - '0') + res * base;
	}
	
	return res;
}
double Message::To_double(const std::string& digits, const int base) {
	int res 		= 0;
	int scale 		= 1;
	bool decimal 	= false;
	double signe 	= 1.0;
	
	auto it = digits.begin();
	if(*it == '-') {
		signe = -1.0;
		++it;
	}
		
	for(; it != digits.end(); ++it) {
		char c = *it;
		if(c != '.') {
			res = (int)(c - '0') + res * base;
			if(decimal) scale *= base;
		}
		else decimal = true;
	}
	
	return signe*res/scale;
}
std::string Message::To_string(const std::vector<char>& vchar) {
	return std::string(vchar.data(), vchar.size());
}
std::vector<char> Message::To_vector(const std::string& str) {
	return std::vector<char>(str.begin(), str.end());
}

// -------------------- Bin message --------------------
BinMessage::BinMessage(const std::vector<char>& serializedData) : Message(), _dataSize(0), _codeAction(0) {
	if(!serializedData.empty())
		unserialize(serializedData.data(), serializedData.size());
}
BinMessage::~BinMessage() {
	clear();
}

// Methods
void BinMessage::unserialize(const char *data, const size_t nb) {	
	if(nb < OFFSET_DATA) {
		std::cout << "Not enough data" << std::endl;
		clear();
		return;
	}
	
	_dataSize 	= Read_256(data+OFFSET_SIZE, SIZE_SIZE);
	_codeAction = Read_256(data+OFFSET_CODE, SIZE_CODE);
	
	if(_dataSize  != nb - OFFSET_DATA){
		std::cout << "Bin message not valide" << std::endl;
		clear();
		return;
	}
	
	_data = std::vector<char>(data+OFFSET_DATA, data+OFFSET_DATA+_dataSize);
}
std::vector<char> BinMessage::serialize() const {
	std::vector<char> dataSerialized;
	
	if(isValide()) {
		auto action256 = Write_256(_codeAction, SIZE_CODE);
		auto dtSize256 = Write_256(_dataSize, SIZE_SIZE);
		
		dataSerialized.reserve(OFFSET_DATA + _dataSize);
		dataSerialized.insert(dataSerialized.end(), dtSize256.begin(), dtSize256.end());
		dataSerialized.insert(dataSerialized.end(), action256.begin(), action256.end());
		dataSerialized.insert(dataSerialized.end(), _data.begin(), _data.end());
	}
	
	return dataSerialized;
}
void BinMessage::clear() {
	_dataSize	= (size_t)0;
	_codeAction = (size_t)0;
	_data.clear();
}

bool BinMessage::isValide() const {
	return (_dataSize == _data.size());
}

// Setters
bool BinMessage::set(const size_t codeAction, const size_t sizeData, const char* data) {
	_codeAction = codeAction;
	_dataSize 	= sizeData;
	_data 		= std::vector<char>(data, data + sizeData);
	
	return isValide();
}
bool BinMessage::set(const size_t codeAction, const std::string& data) {
	return set(codeAction, data.size(), data.data());
}
bool BinMessage::set(const std::string& codeActionStr, const std::string& data) {
	return set(Read_256(codeActionStr), data.size(), data.data());
}

// Getters
const size_t BinMessage::getAction() const {
	return _codeAction;
}
const size_t BinMessage::getSize() const {
	return _dataSize;
}
const std::vector<char>& BinMessage::getData() const {
	return _data;
}

// Statics
std::vector<char> BinMessage::Write_256(size_t valueToConvert, const size_t nbOfSymbole) {
	std::vector<char> values(nbOfSymbole, 0);

	for(int i = (int)nbOfSymbole-1; i >= 0 && valueToConvert > 0; i--) {
		values[i] = valueToConvert % 256;
		valueToConvert /= 256;
	}
	
	return values;
}
size_t BinMessage::Read_256(const char *data, const size_t nb) {
	size_t value = 0;
	for(size_t d = 0; d < nb; d++)
		value = (int)(unsigned char)(*(data+d)) + value*256;
	
	return value;
}
size_t BinMessage::Read_256(const std::vector<char>& valueToConvert) {
	size_t value = 0;
	for(char digit: valueToConvert)
		value = (int)(unsigned char)digit + value*256;
	
	return value;
}
size_t BinMessage::Read_256(const std::string& valueToConvert) {
	return Read_256(valueToConvert.data(), valueToConvert.size());
}

void BinMessage::Println(const BinMessage& msg, const Message::DIRECTION dir) {
	if(dir != Message::S_NONE)
		std::cout << (dir == Message::S_IN ? "<< " : ">> ");
	
	std::cout << msg;
	std::cout << " [At " << 1000.0*clock() / CLOCKS_PER_SEC << " ms.]";
	std::cout << std::endl;	
}


// -------------------- String message --------------------
CmdMessage::CmdMessage(const std::string& serializedMsg) : Message() {
	if(!serializedMsg.empty())
		unserialize(serializedMsg.data(), serializedMsg.size());
}
CmdMessage::~CmdMessage() {
	clear();
}

// Methods
void CmdMessage::unserialize(const char *data, const size_t nb) {
	clear();
	
	// Split msg into commands
	std::istringstream msgFlow(std::string(data, nb));
	std::vector<std::string> commands;
	std::string serialCmd;
	
	while(std::getline(msgFlow, serialCmd, DELIM_CMD))
		commands.push_back(serialCmd);
	
	// Split serial cmd into command
	for(std::string serialCmd : commands) {
		size_t posDelim = serialCmd.find(DELIM_ACT);
		if(posDelim != std::string::npos) {
			addCommand(
				serialCmd.substr(0, posDelim),
				serialCmd.substr(posDelim+1, serialCmd.size() - posDelim - 1)
			);
		}
	}
}

std::vector<char> CmdMessage::serialize() const {
	std::stringstream serialFlow;
	for(Command cmd : _commands) 
		serialFlow << DELIM_CMD << cmd.first << DELIM_ACT << cmd.second;
	
	std::string ss = serialFlow.str();
	return std::vector<char>(ss.begin(), ss.end());
}

void CmdMessage::addCommand(const std::string& action, const std::string& data) {
	addCommand(Command(action, data));
}
void CmdMessage::addCommand(const Command& cmd) {
	_commands.push_back(cmd);
}

const CmdMessage::Command CmdMessage::getCommand(const std::string& action) const {
	for(Command cmd : _commands)
		if(cmd.first == action)
			return cmd;
		
	return Command("NONE", "");
}
const std::vector<CmdMessage::Command>& CmdMessage::getCommands() const {
	return _commands;
}

void CmdMessage::clear() {
	_commands.clear();
}

// Statics
void CmdMessage::Println(const CmdMessage& msg, const Message::DIRECTION dir) {
	if(dir != Message::S_NONE)
		std::cout << (dir == Message::S_IN ? "<< " : ">> ");
	
	std::cout << msg;
	std::cout << " [At " << 1000.0*clock() / CLOCKS_PER_SEC << " ms.]";
	std::cout << std::endl;
}


