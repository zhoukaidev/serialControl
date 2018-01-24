#pragma once
#include "SerialListener.h"
#include <memory>
#include <string>
#include <vector>
#include <thread>
//#include <condition_variable>
#include <mutex>
#include <atomic>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace oym
{
#define SERIAL_LOW_BAUDRATE		115200
#define SERIA_HIGH_BAUDRATE		921600

	class SerialAdapter 
	{
		public:

			SerialAdapter(std::string& com,DWORD nBaud= SERIAL_LOW_BAUDRATE,
				unsigned char nParity = 0,unsigned char nByteSize = 8,unsigned char nStopBit=0);
			SerialAdapter(SerialAdapter&) = delete;
			SerialAdapter& operator=(const SerialAdapter&) = delete;
			SerialAdapter(SerialAdapter&&) = delete;
			SerialAdapter& operator= (SerialAdapter&&) = delete;
			~SerialAdapter();
			void addListener(std::shared_ptr<oym::SerialListener> ptr);
			void removeListener(std::shared_ptr<oym::SerialListener> ptr);
			bool run(void);
			bool stop(void);
		private:
			bool openSerialPort(void);
			bool SetupSerialPort(unsigned short readTimeout = 50);
			void readSerialData(void);
			void detectSerialAvaliable(void);
			bool checkPortAvaliable(void);
			std::string mSerialNum;
			std::vector<std::shared_ptr<oym::SerialListener>> mListeners;
			std::thread mReadThread;
			//std::condition_variable mReadCV;
			std::atomic<bool> mReadThreadQuit = false;
			std::thread mDetectThread;
			//std::condition_variable mDetectCV;
			std::atomic<bool> mDetectThreadQuit = false;
			std::mutex mListenerMTX;
#ifdef _WIN32
			HANDLE mComFile = nullptr;
			DCB mDcb;
#endif
	};
}