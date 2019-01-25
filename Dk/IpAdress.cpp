#include "IpAdress.hpp"

// Constructors
IpAdress::IpAdress(const std::string& ipAndPort) : _type(INVALID_IP) {
	fromFullString(ipAndPort);
}
IpAdress::IpAdress(const std::string& ip, int port) : _type(INVALID_IP) {
	fromString(ip, port);
}
IpAdress::IpAdress(const std::vector<int>& target, int port) : _type(INVALID_IP) {
	fromTarget(target, port);
}
IpAdress::IpAdress(int c1, int c2, int c3, int c4, int port) : _type(INVALID_IP) {
	fromTarget(std::vector<int>{c1, c2, c3, c4}, port);
}
IpAdress::IpAdress(int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8, int port) : _type(INVALID_IP) {
	fromTarget(std::vector<int>{c1, c2, c3, c4, c5, c6, c7, c8}, port);
}

// Methods
std::string IpAdress::toFullString() const {
	if (!isValide())
		return "";

	std::stringstream ss;

	switch (_type) {
		case IpAdress::IP_V4:
			ss << toString();
		break;

		case IpAdress::IP_V6: 		
			ss << "[" << toString() << "]";
		break;
	}

	// Add port
	ss << ":" << _port;
	return ss.str();
}
std::string IpAdress::toString() const {
	if (!isValide())
		return "";

	std::stringstream ss;
	if (_type == IpAdress::IP_V4) {
		ss << std::dec;
		for (int i = 0; i < _target.size() - 1; i++)
			ss << _target[i] << ".";
		ss << _target.back();
	}
	else if (IpAdress::IP_V6) {
		// Count 0s contigus
		ss << std::hex;
		int iBegCompress = -1, iTmpBegCompress = -1;
		int sizeCompress = 0, tmpSizeCompress = 0;

		for (int i = 0; i < _target.size(); i++) {
			// Accumulate
			if (_target[i] == 0) {
				if(tmpSizeCompress == 0)
					iTmpBegCompress = i;
				tmpSizeCompress++;
			}
			
			// Bank
			if (_target[i] != 0 || i == _target.size()-1) {
				if (tmpSizeCompress > sizeCompress && tmpSizeCompress > 1) {
					sizeCompress = tmpSizeCompress;
					iBegCompress = iTmpBegCompress;
				}
				tmpSizeCompress = 0;
			}
		}

		// Write
		if (iBegCompress == 0)
			ss << ":";
			
		for (int i = 0; i < _target.size(); i++) {
			if (i != iBegCompress) {
				ss << _target[i];
				if(i < _target.size() -1) 
					ss << ":";
			}
			else {
				i += sizeCompress-1;
				ss << ":";
			}
		}
	}

	return ss.str();
}

bool IpAdress::fromFullString(const std::string& ipAndPort) {
	// Ip null
	if (ipAndPort.empty()) {
		*this = IpAdress(0, 0, 0, 0, 0);
		return isValide();
	}

	_target.clear();
	_port = 0;

	// Find type based [ or not occurence
	_type = ipAndPort[0] == '[' ? IP_V6 : IP_V4;
	
	// Delimit ip part and port part
	size_t begPos = _type == IP_V4 ? 0 : 1;
	size_t endPos = _type == IP_V4 ? ipAndPort.find_first_of(':') : ipAndPort.find_last_of(':')-1;

	if (endPos == std::string::npos)
		return false;

	std::string strTarget = ipAndPort.substr(begPos, endPos - begPos);
	std::string strPort = ipAndPort.substr(begPos + endPos + 1);

	// Port
	try {
		_port = stoi(strPort, nullptr, 10);
	}
	catch (...) {
		_port = 0;
		return false;
	}

	// Finally
	return fromString(strTarget, _port);
}
bool IpAdress::fromString(const std::string& ip, int port) {
	// Ip null
	if (ip.empty()) {
		*this = IpAdress(0, 0, 0, 0, 0);
		return isValide();
	}

	_port = port;
	_target.clear();
	
	// Find type based on . or : occurence
	for (char c : ip) {
		if (c == '.') {
			_type = IP_V4;
			break;
		}
		if (c == ':') {
			_type = IP_V6;
			break;
		}
	}
	if (_type == INVALID_IP)
		return false;

	// Populate target
	char delimit = _type == IP_V4 ? '.' : ':';
	int base	 = _type == IP_V4 ? 10 : 16;

	std::istringstream flow(ip);
	std::string s;
	int iCompress = -1;
	int i = 0;

	while (getline(flow, s, delimit)) {
		if (_type == IP_V6 && s.empty()) {
			if (iCompress <= 0) { // found ::
				if(iCompress == -1)
					iCompress = i;
				continue;
			}
			else { // multiple ::, bad
				_target.clear();
				return false;
			}
		}

		try {
			_target.push_back(stoi(s, nullptr, base));
		}
		catch (...) {
			_target.clear();
			return false;
		}
		i++;
	}
	
	
	// Was compressed
	if (_type == IP_V6 && iCompress > -1) {
		int nToAdd = 8 - (int)_target.size();
		if (nToAdd < 0)
			return false;

		std::vector<int> pad((size_t)nToAdd, (int)0);
		_target.insert(_target.begin() + iCompress, pad.begin(), pad.end());
	}

	return isValide();
}
bool IpAdress::fromTarget(const std::vector<int>& target, int port) {
	if(target.size() == 4) {
		_type = IP_V4;
		_target = target;
	}
	else if (target.size() == 8) {
		_type = IP_V6;
		_target = target;
	}
	_port = port;

	return isValide();
}

// Getters
bool IpAdress::isValide() const {
	// Type
	switch (_type) {
		case IpAdress::IP_V4:
			if (_target.size() != 4)
				return false;
			break;
		case IpAdress::IP_V6:
			if (_target.size() != 8)
				return false;
			break;
		default:
			return false;
	}

	// Port
	if (_port < 0 || _port > 65535)
		return false;

	// Target
	const int max_T = getMaxT(_type);
	for (int t : _target) {
		if (t < 0 || t > max_T) {
			return false;
		}
	}

	return true;
}
bool IpAdress::isNull() const {
	if (_port != 0)
		return false;

	for (int t : _target) {
		if (t != 0) {
			return false;
		}
	}

	return true;
}

int IpAdress::getPort() const {
	return _port;
}
const std::vector<int>& IpAdress::getTarget() const {
	return _target;
}
IpAdress::IP_ADRESS_TYPE IpAdress::getType() const {
	return _type;
}
const std::vector<unsigned char> IpAdress::getChars() const {
	std::vector<unsigned char> chars;
	if (_type == IP_V4) {
		for (int c : _target) {
			chars.push_back((unsigned char)c);
		}
	}
	else if(_type == IP_V6) {
		for (size_t i = 0; i < _target.size(); i++) {
			chars.push_back((unsigned char)(_target[i] / 256));
			chars.push_back((unsigned char)(_target[i] % 256));
		}
	}
	
	return chars;
}

// Setters
bool IpAdress::setPort(int p) {
	_port = p;
	return isValide();
}

// Statics
int IpAdress::getMaxT(IP_ADRESS_TYPE type) {
	if (type == IP_V4)
		return 255;
	if (type == IP_V6)
		return 65535;
	return 0;
}
IpAdress IpAdress::localhost(IP_ADRESS_TYPE type) {
	if (type == IP_V4)
		return IpAdress("127.0.0.1", 0);
	if (type == IP_V6)
		return IpAdress("::1", 0);
	return IpAdress("");
}

// Operations
IpAdress IpAdress::operator+(int add) {
	if (!isValide() || add <= 0)
		return *this;

	const int mod_T = 1 + getMaxT(_type);
	auto newTarget = _target;
	int i = (int)newTarget.size()-1;

	int q = add;
	while (i >= 0 && q > 0) {
		int total = newTarget[i] + q;
		newTarget[i] = total % mod_T;
		q = total / mod_T;
		i--;
	}
	
	return IpAdress(newTarget, _port);
}
bool IpAdress::operator>(const IpAdress& ipCompared) const {
	if (_type != ipCompared.getType())
		return false;

	auto otherTarget = ipCompared.getTarget();
	for (int i = 0; i < _target.size(); i++) {
		if (_target[i] > otherTarget[i])
			return true;
		else if (_target[i] < otherTarget[i])
			return false;
	}

	return _port > ipCompared.getPort();
}
bool IpAdress::operator==(const IpAdress& ipCompared) const {
	return !((*this > ipCompared) || (ipCompared > *this));
}
bool IpAdress::operator!=(const IpAdress& ipCompared) const {
	return !(*this == ipCompared);
}
