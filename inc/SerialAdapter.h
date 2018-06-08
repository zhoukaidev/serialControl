#pragma once
#include "SerialListener.h"
#include <memory>
#include <string>
#include <vector>
#include <vector>
#include <thread>
#include <mutex>
#include <future>
#include <atomic>
#include <set>
#include <queue>
#include <future>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace oym
{
#define SERIAL_LOW_BAUDRATE		115200
#define SERIA_HIGH_BAUDRATE		921600

	class noncopyable {
		protected:
			noncopyable() = default;
			~noncopyable() = default;
			noncopyable(const noncopyable&) = delete;
			noncopyable& operator=(const noncopyable&) = delete;
	};
	
	enum ERROR_STATUS {
		SUCCESS,
		FAILURE,
		BAD_PARAMETER,
		HAS_OPENED,
		HAS_CLOSED,
		HAS_RUN
	};
	typedef ERROR_STATUS RET_CODE;

	template <class T>
	class Thread_safe_Queue {
	public:
		Thread_safe_Queue() {

		}
		~Thread_safe_Queue() {

		}
		void push(T val) {
			std::lock_guard<std::mutex> lgx(mQueueMx);
			mdataQueue.push(val);
			mQueueCv.notify_one();
		}

		T wait_and_pop() {
			std::unique_lock<std::mutex> lk(mQueueMx);
			mQueueCv.wait(lk, [this] {return !mdataQueue.empty(); });
			T retval = mdataQueue.front();
			mdataQueue.pop();
			return retval;
		}
		bool empty() {
			std::lock_guard(mQueueMx);
			return mdataQueue.empty();
		}
	private:
		std::queue<T> mdataQueue;
		std::mutex mQueueMx;
		std::condition_variable mQueueCv;
	};

	class serial :public noncopyable
	{
		public:
			virtual RET_CODE open(unsigned int port, unsigned long nBaud, unsigned char nParity,
				unsigned char nByteSize, unsigned char nStopBit) = 0;
			virtual RET_CODE close() = 0;
			virtual RET_CODE write_sync(unsigned char data) = 0;
			virtual std::future<bool> write_async(unsigned char data) = 0;
			virtual RET_CODE write_sync(std::shared_ptr<std::vector<unsigned char>> data) = 0;
			virtual std::future<bool> write_async(std::shared_ptr<std::vector<unsigned char>> data) = 0;
			virtual RET_CODE registerListener(std::weak_ptr<oym::SerialListener> ptr) = 0;
			virtual RET_CODE unRegisterListener(std::weak_ptr<oym::SerialListener> ptr) = 0;
			virtual std::shared_ptr<std::vector<std::string>> enumSerial(void) = 0;
		protected:
			serial(){}
			virtual ~serial(){}
	};


	class serialAdapter : public serial
	{
		public:
			serialAdapter():serial(),mSerialOpend(false){}
			~serialAdapter() {};
			virtual RET_CODE open(unsigned int port, 
				unsigned long nBaud = SERIAL_LOW_BAUDRATE, 
				unsigned char nParity = 0,
				unsigned char nByteSize = 8, 
				unsigned char nStopBit = 1) override;

			virtual RET_CODE close()override;
			virtual RET_CODE write_sync(unsigned char data) override;
			virtual std::future<bool> write_async(unsigned char data)override;
			virtual RET_CODE write_sync(std::shared_ptr<std::vector<unsigned char>> data) override;
			virtual std::future<bool> write_async(std::shared_ptr<std::vector<unsigned char>> data)  override;
			virtual RET_CODE registerListener(std::weak_ptr<oym::SerialListener> ptr) override;
			virtual RET_CODE unRegisterListener(std::weak_ptr<oym::SerialListener> ptr) override;
			virtual std::shared_ptr<std::vector<std::string>> enumSerial(void) override;

		private:
			RET_CODE openSerialPort(void);
			RET_CODE closeSerialPort(void);
			RET_CODE SetupSerialPort(HANDLE &file,unsigned short readTimeout = 50);
			void readSerialData(void);
			void detectSerialAvaliable(void);
			RET_CODE checkPortAvaliable(void);
			RET_CODE quitDetectThread(void);
			RET_CODE quitReadThread(void);
			RET_CODE writeData(unsigned char* ptr, std::size_t size);
			std::mutex mListenerMtx;
			std::set <std::weak_ptr<SerialListener>,std::owner_less<std::weak_ptr<SerialListener>>> mListener;
			
			std::thread mReadThread;
			std::atomic<bool> mReadThreadQuit = false;
			std::thread mDetectThread;
			std::atomic<bool> mDetectThreadQuit = false;

			std::atomic<bool> mSerialOpend;

			unsigned char mParity;
			unsigned char mByteSize;
			unsigned char mStopBit;
			unsigned int mComPort;
			unsigned long mBaudRate;
#ifdef _WIN32
			HANDLE mComFile = nullptr;
			DCB mDcb;
#endif
	};
}