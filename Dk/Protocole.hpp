#ifndef PROTOCOLE_H
#define PROTOCOLE_H

#include <string.h>
#include <sstream>
#include <vector>
#include <utility>
#include <iostream>
#include <time.h>

#include "ProtocoleActionCode.hpp"

namespace Protocole {	
	// Base messages
	class Message {
	public:
		// Enums
		enum DIRECTION {
			S_NONE, S_IN, S_OUT
		};
		
		// Constructeurs
		Message();
		virtual ~Message() = 0;
		
		// Methods
		virtual void unserialize(const char *data, const size_t nb) = 0;
		virtual std::vector<char> serialize() const = 0;
		
		virtual void clear() = 0;
		
		// Statics
		static size_t To_unsignedInt(const std::string& digits, const int base = 10);
		static double To_double(const std::string& digits, const int base = 10);
		static std::string To_string(const std::vector<char>& vchar);
		static std::vector<char> To_vector(const std::string& vchar);

	};
	
	// Used for socket message
	class BinMessage : public Message {
	public:
		// Define some specifics words
		static const size_t SIZE_SIZE = 4;
		static const size_t SIZE_CODE = 4;
		
		static const size_t OFFSET_SIZE = 0;
		static const size_t OFFSET_CODE = OFFSET_SIZE + SIZE_SIZE;
		static const size_t OFFSET_DATA = OFFSET_CODE + SIZE_CODE;
		
		// Constructor
		BinMessage(const std::vector<char>& serializedData = std::vector<char>());
		virtual ~BinMessage();
			
		// Methods
		void unserialize(const char *data, const size_t nb) 	override;
		std::vector<char> serialize() const 						override;
		void clear() 															override;
		
		bool isValide() const;
		
		// Setters
		bool set(const size_t codeAction, const size_t sizeData, const char* data);
		bool set(const size_t codeAction, const std::string& data);
		bool set(const std::string& codeAction, const std::string& data);
		
		// Getters
		const size_t getAction() const;
		const size_t getSize() const;
		const std::vector<char>& getData() const;
		
		// Statics
		static std::vector<char> Write_256(size_t valueToConvert, const size_t nbOfSymbole);
		static size_t Read_256(const char *data, const size_t nb);
		static size_t Read_256(const std::vector<char>& valueToConvert);
		static size_t Read_256(const std::string& valueToConvert);
		
		static void Println(const BinMessage& msg, const Message::DIRECTION dir = Message::S_NONE);
		
		// Operators
		friend std::ostream& operator<<(std::ostream& os, const BinMessage& msg) {
			if(!msg.isValide())
				os << "{Invalide msg}";
			else
				os << "{" << std::string(msg._data.data(), msg._dataSize) << " [msg: " << msg._dataSize << "bytes]" << "} ";	
			return os;
		}
		
	private:
		// Members
		size_t _dataSize; 		// Number of bytes in _data
		size_t _codeAction;
		std::vector<char> _data;
	};
	
	
	
	// Used for higher level message
	class CmdMessage : public Message {
	public:
		// Define some specifics words
		static const char DELIM_CMD = '#';
		static const char DELIM_ACT = ':';
		
		typedef std::pair<std::string, std::string> Command;
	
		// Constructor
		CmdMessage(const std::string& serializedMsg = std::string());
		virtual ~CmdMessage();
		
		// Methods
		void unserialize(const char *data, const size_t nb) override;
		std::vector<char> serialize() const 				override;
		void clear() 										override;
		
		void addCommand(const std::string& action, const std::string& data);
		void addCommand(const Command& cmd);
		
		const Command getCommand(const std::string& action) const;
		const std::vector<Command>& getCommands() const;
		
		// Statics
		static void Println(const CmdMessage& msg, const Message::DIRECTION dir = Message::S_NONE);
		
		// Operators
		friend std::ostream& operator<<(std::ostream& os, const CmdMessage& msg) {
			for(Command cmd : msg._commands)
				os << "{"<< cmd.first << " -> " << cmd.second << " [" << cmd.second.size() << "bytes]" << "} ";
			
			return os;
		}
		
	private:
		// Members
		std::vector<Command> _commands;
	};
	
	
	// Format
	struct FormatStream {
		// Constructor
		FormatStream(int h = 0, int w = 0, int c = 0);
		explicit FormatStream(const CmdMessage& cmd);
		
		// Methods
		bool isEmpty() const;
		
		void fromCmd(const CmdMessage& cmd);
		const CmdMessage toCmd() const;
		
		// Datas
		int height;
		int width;
		int channels;
		int fps;
		double hue;
		double saturation;
		double brightness;
		double contrast;
		double exposure;
		double autoExposure;
		std::string name;
	};
}

#endif
