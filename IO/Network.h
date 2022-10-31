/*
	Copyright(c) 2021-2022 jvde.github@gmail.com

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <list>
#include <thread>
#include <mutex>

#ifdef HASCURL
#include <curl/curl.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#define SOCKET		int
#define closesocket close
#endif

#ifdef __ANDROID__
#include <netinet/in.h>
#endif

#include "Stream.h"
#include "Common.h"
#include "AIS.h"
#include "JSON.h"
#include "TCP.h"
#include "Utilities.h"
#include "ZIP.h"

namespace IO {

	class HTTP : public JSONbuildString, public Setting {

#ifdef HASCURL

		std::thread run_thread;
		bool terminate = false, running = false;
		std::mutex queue_mutex;

		ZIP zip;
		std::string msg;
		bool gzip = false;

		std::string url, userpwd;
		int INTERVAL = 30;
		int TIMEOUT = 10;

		std::string stationid;

		std::string model, model_setting;
		std::string product, vendor, serial, device_setting;

		bool show_response = true;

		char response[1024];

		enum class PROTOCOL{ AISCATCHER, APRS } protocol = PROTOCOL::AISCATCHER;

		static size_t curl_cb(char* contents, size_t size, size_t nmemb, char* s) {
			int len = MIN(size * nmemb, 1023);
			std::memcpy(s, contents, len);
			s[len] = '\0';
			return len;
		}

		void send(const std::string&, const std::string&);
		void post();
		void process();

	private:
		std::list<std::string> queue;

	public:
		~HTTP() {
			stopServer();
		}
#endif
	public:
		void Ready() {
#ifdef HASCURL
			const std::lock_guard<std::mutex> lock(queue_mutex);
			queue.push_back(json);
#endif
		}

		virtual void Set(std::string option, std::string arg);

		void startServer() {
#ifdef HASCURL
			if (!running) {

				running = true;
				terminate = false;

				run_thread = std::thread(&HTTP::process, this);
				std::cerr << "HTTP: start server (" << url << ")." << std::endl;
			}
#else
			throw "HTTP: not implemented, please recompile with libcurl support.";
#endif
		}

		void stopServer() {
#ifdef HASCURL
			if (running) {

				running = false;
				terminate = true;
				run_thread.join();

				std::cerr << "HTTP: stop server (" << url << ")." << std::endl;
			}
#endif
		}
	};

	class UDPEndPoint {
		std::string address;
		std::string port;

		int sourceID = -1;

	public:
		friend class UDP;
		friend class TCP;

		UDPEndPoint(std::string a, std::string p, int id = -1) {
			address = a, port = p;
			sourceID = id;
		}
		int ID() { return sourceID; }
	};

	class UDP : public StreamIn<AIS::Message> {
		SOCKET sock = -1;
		struct addrinfo* address = NULL;

	public:
		~UDP();
		UDP();

		void Receive(const AIS::Message* data, int len, TAG& tag);
		void openConnection(const std::string& host, const std::string& port);
		void openConnection(UDPEndPoint& u) { openConnection(u.address, u.port); }
		void closeConnection();
	};

	class TCP : public StreamIn<AIS::Message> {
		::TCP::Client con;

	public:
		void Receive(const AIS::Message* data, int len, TAG& tag);
		void openConnection(const std::string& host, const std::string& port);
		void openConnection(UDPEndPoint& u) { openConnection(u.address, u.port); }
		void closeConnection();
	};

}