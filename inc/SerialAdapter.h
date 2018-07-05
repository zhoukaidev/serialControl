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
			std::lock_guard<std::mutex> lk(mQueueMx);
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
			std::lock_guard<std::mutex> lk(mQueueMx);
			return mdataQueue.empty();
		}
		void clear() {
			std::lock_guard<std::mutex> lk(mQueueMx);
			while (!mdataQueue.empty())
				mdataQueue.pop();
		}
	private:
		std::queue<T> mdataQueue;
		std::mutex mQueueMx;
		std::condition_variable mQueueCv;
	};

	class serial :public noncopyable
	{
		public:
			enum StopBits{
				STOP_BITS_1,
				STOP_BITS_1_5,
				STOP_BITS_2
			};
			enum Parity {
				EVEN_PARITY,
				MARK_PARITY,
				NO_PARITY,
				ODD_PARITY,
				SPACE_PARITY
			};
			enum DataBits {
				DATA_BITS_5,
				DATA_BITS_6,
				DATA_BITS_7,
				DATA_BITS_8
			};
		public:
			virtual RET_CODE open(unsigned int port,
				unsigned long nBaud,
				Parity nParity,
				DataBits nByteSize,
				StopBits stop) = 0;
			virtual RET_CODE close() = 0;
			virtual RET_CODE write_sync(unsigned char data) = 0;
			virtual RET_CODE write_sync(std::shared_ptr<std::vector<unsigned char>> data) = 0;
			virtual RET_CODE write_async(unsigned char data) = 0;
			virtual RET_CODE write_async(std::shared_ptr<std::vector<unsigned char>> data) = 0;
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
			~serialAdapter();
			virtual RET_CODE open(unsigned int port, 
				unsigned long nBaud = SERIAL_LOW_BAUDRATE, 
				Parity nParity = Parity::NO_PARITY,
				DataBits nByteSize = DataBits::DATA_BITS_8, 
				StopBits nStopBit = StopBits::STOP_BITS_1) override;

			virtual RET_CODE close()override;
			virtual RET_CODE write_sync(unsigned char data) override;
			virtual RET_CODE write_sync(std::shared_ptr<std::vector<unsigned char>> data) override;
			virtual RET_CODE write_async(unsigned char data)override;
			virtual RET_CODE write_async(std::shared_ptr<std::vector<unsigned char>> data)  override;
			virtual RET_CODE registerListener(std::weak_ptr<oym::SerialListener> ptr) override;
			virtual RET_CODE unRegisterListener(std::weak_ptr<oym::SerialListener> ptr) override;
			virtual std::shared_ptr<std::vector<std::string>> enumSerial(void) override;

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