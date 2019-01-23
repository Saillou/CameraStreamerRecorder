#ifndef VIDEO_STREAM_WRITER_HPP
#define VIDEO_STREAM_WRITER_HPP

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

#include "../Globals/structures.hpp"
#include "Protocole.hpp"
#include "ManagerConnection.hpp"
#include "Chronometre.hpp"


namespace Dk {

	class VideoStreamWriter {
	public:
		// Constructors
		VideoStreamWriter(ManagerConnection& managerConnection, const int port, const std::string& name = "WiFi device");
		~VideoStreamWriter();
		
		// Methods
		const Protocole::FormatStream& startBroadcast(const Gb::Size& size, int channels);
		bool update(const Gb::Frame& frame);
		void release();
		
		// Thread launch methods
		void handleClients();
		
		// Getters
		bool isValide() const;
		
	private:
		// Methods
		void _handleClient(int idClient);
		bool _treatClient(int idClient, const Protocole::BinMessage& msg);
		bool _changeFormat(const Protocole::FormatStream& newFormat);
		
		// Members
		std::shared_ptr<Server> _server;
		bool _valide;
		
		Protocole::FormatStream _format;
		
		Gb::Frame _frameToSend;
		std::thread* _threadClients;

		std::mutex _mutexFrame;
		std::atomic<bool> _atomRunning{false};
		std::atomic<bool> _atomImageUpdated{false};
		
		
	};
}

#endif
