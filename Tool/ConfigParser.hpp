#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <map>
#include <utility>

class ConfigParser {
public:
	// Nested sub-class
	class Item {
	public:
		// Constructors
		explicit Item(const std::string& rawData = "");
		
		// Methods
		double toNumber(int base = 10) const;
		bool toBool() const;
		std::string toString() const;
		
	private:
		// Members
		std::string _rawData;
	};
	
	// Types
	typedef std::map<std::string, Item> Result;
	
	// Static methods
	static Result parse(const std::string& path);
	
private:
	// Types
	typedef std::pair<std::string, Item> _MapItem;
	
	// Static methods
	static _MapItem _parseLine(std::string line);
};

#endif
