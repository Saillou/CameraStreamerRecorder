#include "ConfigParser.hpp"

// --- Main class ---
// Public static methods
ConfigParser::Result ConfigParser::parse(const std::string& path) {
	Result result;
	
	std::ifstream fileIn(path.c_str());
	if(!fileIn.is_open()) {
		std::cout << "Could not open " << path << std::endl;
		return result;
	}
	
	for(std::string line; std::getline(fileIn, line);) {
		_MapItem mpi = _parseLine(line);
		
		if(!mpi.first.empty())
			result.insert(mpi);
	}
	
	fileIn.close();
	return result;
}

// Private static methods
ConfigParser::_MapItem ConfigParser::_parseLine(std::string line) {
	// Discard lines
	if(line.empty()) 	// nothing
		return _MapItem("", Item(""));
	if(line[0] == '#') 	// comment
		return _MapItem("", Item(""));
	if(line[0] == '\0' || line[0] == '\r' || line[0] == '\n') // return
		return _MapItem("", Item(""));
	
	// Find joncture
	std::size_t pos = line.find(':');
	if(pos == std::string::npos)
		return _MapItem("", Item(""));
	
	// Remove rn
	while(!line.empty() && (line.back() == '\r' || line.back() == '\n'))
		line.pop_back();
	
	return _MapItem(line.substr(0, pos), Item(line.substr(pos+1)));
}


// --- Nested sub-class ---
// Constructors
ConfigParser::Item::Item(const std::string& rawData) : _rawData(rawData) {
	// Nothing else
}


// Public methods
double ConfigParser::Item::toNumber(int base) const {
	int res 		= 0;
	int scale 		= 1;
	bool decimal 	= false;
	double signe 	= 1.0;
	
	// Signe
	auto it = _rawData.begin();
	if(*it == '-') {
		signe = -1.0;
		++it;
	}
		
	// Numerical
	for(; it != _rawData.end(); ++it) {
		char c = *it;
		if(c != '.') {
			// Check error
			if(c < '0' || c > '9')
				return 0;
				
			// Ok
			res = (int)(c - '0') + res * base;
			
			if(decimal) 
				scale *= base;
		}
		else 
			decimal = true;
	}
	
	return signe*res/scale;
}
bool ConfigParser::Item::toBool() const {
	return _rawData == "True";
}
std::string ConfigParser::Item::toString() const {
	if(_rawData.size() < 3)
		return "";
	if(_rawData.front() != '"' || _rawData.back() != '"')
		return "";
		
	return _rawData.substr(1, _rawData.size()-2);
}

