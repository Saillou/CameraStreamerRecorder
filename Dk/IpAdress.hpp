#ifndef IP_ADRESS_HPP
#define IP_ADRESS_HPP

#include <string>
#include <vector>
#include <sstream>

class IpAdress {
public:
	enum IP_ADRESS_TYPE {
		INVALID_IP, IP_V4, IP_V6
	};
public:
	// Constructors
	IpAdress(const std::string& ipAndPort);
	IpAdress(const std::string& ip, int port);
	IpAdress(const std::vector<int>& target, int port);
	IpAdress(int c1, int c2, int c3, int c4, int port);
	IpAdress(int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8, int port);

	// Methods
	std::string toFullString() const;
	std::string toString() const;

	bool fromFullString(const std::string&);
	bool fromString(const std::string&, int);
	bool fromTarget(const std::vector<int>& target, int);

	// Getters
	bool isValide() const;
	bool isNull() const;
	static int getMaxT(IP_ADRESS_TYPE);
	static IpAdress localhost(IP_ADRESS_TYPE);

	int getPort() const;
	const std::vector<int>& getTarget() const;
	const std::vector<unsigned char> getChars() const;
	IP_ADRESS_TYPE getType() const;

	// Setters
	bool setPort(int);

	// Operations
	IpAdress operator+(int);
	bool operator>(const IpAdress&) const;
	bool operator==(const IpAdress&) const;
	bool operator!=(const IpAdress&) const;

private:
	// Members
	std::vector<int> _target;
	int _port;
	IP_ADRESS_TYPE _type;
};

#endif
