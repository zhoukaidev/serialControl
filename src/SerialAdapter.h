#pragma once
#include <string>
#include <thread>
#include <future>
#include <atomic>
#include <set>
#include "scl\serial.h"
#include "util.h"
#ifdef _WIN32
#include <Windows.h>
#endif

namespace scl 
{
	class serialAdapter : public serial
	{
		public:
			serialAdapter():serial(),mSerialOpend(false){}
			~serialAdapter();
			RET_CODE open(unsigned int port, 
				unsigned long nBaud = SERIAL_LOW_BAUDRATE, 
				Parity nParity = Parity::NO_PARITY,
				DataBits nByteSize = DataBits::DATA_BITS_8, 
				StopBits nStopBit = StopBits::STOP_BITS_1) override;

			RET_CODE close()override;
			RET_CODE write_sync(unsigned char data) override;
			RET_CODE write_sync(std::shared_ptr<std::vector<unsigned char>> data) override;
			RET_CODE write_async(unsigned char data)override;
			RET_CODE write_async(std::shared_ptr<std::vector<unsigned char>> data)  override;
			RET_CODE registerListener(std::weak_ptr<scl::SerialListener> ptr) override;
			RET_CODE unRegisterListener(std::weak_ptr<scl::SerialListener> ptr) override;
			std::shared_ptr<std::vector<std::string>> enumSerial(void) override;

		private:
			RET_CODE openSerialPort(void);
			RET_CODE closeSerialPort(void);
#ifdef _WIN32
			RET_CODE SetupSerialPort(HANDLE file,
					unsigned long baud = SERIAL_LOW_BAUDRATE,
					Parity nParity = Parity::NO_PARITY,
					DataBits nDatabits = DataBits::DATA_BITS_8,
					StopBits nStopbits = StopBits::STOP_BITS_1,
					unsigned short readTimeout = 50);
			void syncParity(DCB& dcb, Parity parity);
			void syncDataBits(DCB& dcb, DataBits databit);
			void syncStopBits(DCB& dcb, StopBits stopbit);
#else
			RET_CODE SetupSerialPort(unsigned char readTimeout);
#endif
			void readSerialData(void);
			void detectSerialAvaliable(void);
			void writeSerialData(void);
			RET_CODE checkPortAvaliable(void);
			RET_CODE quitDetectThread(void);
			RET_CODE quitReadThread(void);
			RET_CODE quitWriteThread(void);
			RET_CODE writeData(unsigned char* ptr, std::size_t size);
			std::mutex mListenerMtx;
			std::set <std::weak_ptr<SerialListener>,std::owner_less<std::weak_ptr<SerialListener>>> mListener;
			
			std::thread mReadThread;
			std::atomic<bool> mReadThreadQuit = false;
			std::thread mDetectThread;
			std::atomic<bool> mDetectThreadQuit = false;
			std::thread mWriteThread;
			std::atomic<bool> mWriteThreadQuit = false;
			std::mutex mWriteMtx;

			std::atomic<bool> mSerialOpend;

			Parity mParity;
			DataBits mByteSize;
			StopBits mStopBit;
			unsigned int mComPort;
			unsigned long mBaudRate;

			Thread_safe_Queue<std::shared_ptr<std::vector<unsigned char>>> mQueue;
#ifdef _WIN32
			HANDLE mComFile = nullptr;
#endif
	};
}
