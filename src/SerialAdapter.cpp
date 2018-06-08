#include "SerialAdapter.h"
#include <algorithm>
#include <cctype>
#include <atlstr.h>
#include <iostream>
#include <stdexcept>
#include <functional>

using std::mutex;
using std::weak_ptr;
using std::set;

namespace oym
{

	RET_CODE serialAdapter::registerListener(weak_ptr<SerialListener> ptr)
	{
		auto sp = ptr.lock();
		if (nullptr == sp)
			return BAD_PARAMETER;
		std::lock_guard<mutex> mtx(mListenerMtx);
		auto ret = mListener.insert(ptr);
		if (ret.second)
			return SUCCESS;
		else
			return FAILURE;
	}

	RET_CODE serialAdapter::unRegisterListener(weak_ptr<SerialListener> ptr)
	{
		auto sp = ptr.lock();
		if (nullptr == sp)
			return BAD_PARAMETER;
		std::lock_guard<mutex> mtx(mListenerMtx);
		auto eraseSize = mListener.erase(ptr);
		if (eraseSize)
			return SUCCESS;
		else
			return FAILURE;
	}
	inline RET_CODE	serialAdapter::quitReadThread(void)
	{
		mReadThreadQuit = true;
		if (mReadThread.joinable())
			mReadThread.join();
		mReadThreadQuit = false;
		return SUCCESS;
	}
	inline RET_CODE serialAdapter::quitDetectThread(void)
	{
		mDetectThreadQuit = true;
		if (mDetectThread.joinable())
			mDetectThread.join();
		mDetectThreadQuit = false;
		return SUCCESS;
	}
	RET_CODE serialAdapter::open(unsigned int port,
		unsigned long nBaud,
		unsigned char nParity,
		unsigned char nByteSize,
		unsigned char nStopBit)
	{
		if (true == mSerialOpend.load())
			return SUCCESS;
		mComPort = port;
		mDcb.BaudRate = nBaud;
		mDcb.Parity = nParity;
		mDcb.ByteSize = nByteSize;
		mDcb.StopBits = ONESTOPBIT;
		std::cout << "port: " << (unsigned int)port << std::endl;
		std::cout << "nBaud: " << mDcb.BaudRate << std::endl;
		std::cout << "Parity: " <<(int)mDcb.Parity << std::endl;
		std::cout << "ByteSize: " << (int)mDcb.ByteSize << std::endl;
		std::cout << "StopBits: " <<(int)mDcb.StopBits << std::endl;
		if (std::this_thread::get_id() == mReadThread.get_id() ||
			std::this_thread::get_id() == mDetectThread.get_id()) {
			return FAILURE; 
		}
		if (SUCCESS == openSerialPort()) {
			quitReadThread();
			quitDetectThread();
			mReadThread = std::thread(std::mem_fn(&serialAdapter::readSerialData),this);
			mDetectThread = std::thread(std::mem_fn(&serialAdapter::detectSerialAvaliable), this);
			mSerialOpend = true;
			return SUCCESS;
		}
		return FAILURE;

	}

	RET_CODE serialAdapter::close()
	{
		if (false == mSerialOpend.load())
			return SUCCESS;
		if (std::this_thread::get_id() == mReadThread.get_id()||
			std::this_thread::get_id() == mDetectThread.get_id()) {
			return FAILURE;
		}
		quitReadThread();
		quitDetectThread();
		closeSerialPort();
		mSerialOpend = false;
		return SUCCESS;
	}

	RET_CODE serialAdapter::openSerialPort(void)
	{
#ifdef _WIN32
		CString sCom;
		sCom.Format(_T("\\\\.\\COM%d"), mComPort);
		std::wcout << sCom << std::endl;
		mComFile= CreateFile(sCom.GetBuffer(50),
			GENERIC_READ | GENERIC_WRITE,
			0,/* do not share*/
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL);
		if (mComFile == INVALID_HANDLE_VALUE) {
			return FAILURE;
		}
		if (SUCCESS != SetupSerialPort(mComFile,50)) {
			CloseHandle(mComFile);
			return FAILURE;
		}
		return SUCCESS;
#else
		return FAILURE;
#endif
	}

	RET_CODE serialAdapter::closeSerialPort(void)
	{
#ifdef _WIN32
		bool ret = CloseHandle(mComFile);
		if (ret)
			return SUCCESS;
		else
			return FAILURE;
#endif
	}

	RET_CODE serialAdapter::SetupSerialPort(HANDLE &file,unsigned short readTimeout)
	{
#ifdef _WIN32
		DCB ndcb;
		COMMTIMEOUTS timeouts;
		timeouts.ReadIntervalTimeout = 0;
		timeouts.ReadTotalTimeoutMultiplier = 0;
		timeouts.ReadTotalTimeoutConstant = readTimeout;
		timeouts.WriteTotalTimeoutConstant = 0;
		timeouts.WriteTotalTimeoutMultiplier = 0;
		SetCommTimeouts(file, &timeouts);
		if (!GetCommState(file, &ndcb)) {
			throw std::runtime_error("GetCommState failed\n");
		}

		ndcb.DCBlength = sizeof(DCB);
		ndcb.BaudRate = mDcb.BaudRate;
		ndcb.Parity = mDcb.Parity;
		ndcb.ByteSize = mDcb.ByteSize;
		ndcb.StopBits = mDcb.StopBits;
		ndcb.fRtsControl = RTS_CONTROL_DISABLE;
		ndcb.fDtrControl = DTR_CONTROL_ENABLE;
		ndcb.fOutxCtsFlow = FALSE;
		ndcb.fOutxDsrFlow = FALSE;
		ndcb.fOutX = FALSE;
		ndcb.fInX = FALSE;

		if (!SetCommState(file, &ndcb)) {
			throw std::runtime_error("SetCommState failed\n");
		}
		
		PurgeComm(file, PURGE_RXCLEAR | PURGE_TXCLEAR);

		/*clear error*/
		DWORD dwError;
		COMSTAT cs;
		if (!ClearCommError(file, &dwError, &cs)) {
			throw std::runtime_error("ClearCommError failed\n");
		}

		/*set mask*/
		SetCommMask(file, EV_RXCHAR);

		return SUCCESS;
#else
		return FAILURE;
#endif
	}



	void serialAdapter::readSerialData(void)
	{
#ifdef _WIN32
		OVERLAPPED osRead;
		memset(&osRead, 0, sizeof(OVERLAPPED));
		osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		bool loop = true;
		while (loop) {
			DWORD readLen = 0;
			 unsigned char data = 0x00;
			if (mReadThreadQuit.load() == true) {
				return;
			}
			if (!ReadFile(mComFile, &data, 1, &readLen, &osRead)) {
				if (GetLastError() == ERROR_IO_PENDING) {
					GetOverlappedResult(mComFile, &osRead, &readLen, true);
				}
			}
			if (readLen) {
				std::lock_guard<std::mutex> ltx(mListenerMtx);
				for (auto listen : mListener) {
					auto lock = listen.lock();
					if (lock)
						lock->onData(data);
				}
			}
		}
#endif
	}

	RET_CODE serialAdapter::checkPortAvaliable(void)
	{
#ifdef _WIN32
		HANDLE comFile;
		CString sCom;
		sCom.Format(_T("\\\\.\\COM%d"), mComPort);
		comFile = CreateFile(sCom.GetBuffer(50),
			GENERIC_READ | GENERIC_WRITE,
			0,/* do not share*/
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL);
		if (comFile == INVALID_HANDLE_VALUE) {
			if (ERROR_ACCESS_DENIED == GetLastError()) {
				return FAILURE;
			}
			else if (ERROR_FILE_NOT_FOUND == GetLastError()) {
				return SUCCESS;
			}
		}
		else {
			CloseHandle(comFile);
			return SUCCESS;
		}
		return FAILURE;
#endif
	}

	void serialAdapter::detectSerialAvaliable(void)
	{
#ifdef _WIN32
		bool loop = true;
		while (loop) {
			if (mDetectThreadQuit.load() == true) {
				return;
			}
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			if (SUCCESS == checkPortAvaliable()) {
				{
					std::lock_guard<std::mutex> ltx(mListenerMtx);
					for (auto listener : mListener) {
						auto f = listener.lock();
						if (f) {
							f->onClosed();
						}
					}
				}
				quitReadThread();
				return;
			}
#endif
		}
	}
	std::shared_ptr<std::vector<std::string>> serialAdapter::enumSerial(void)
	{
		return std::shared_ptr<std::vector<std::string>>();
	}
	RET_CODE serialAdapter::writeData(unsigned char *ptr, std::size_t size)
	{
#ifdef _WIN32
		DWORD writeByte = 0;
		OVERLAPPED osWrite;
		memset(&osWrite, 0,sizeof(osWrite));
		if (!WriteFile(mComFile, ptr, static_cast<WORD>(size), &writeByte, &osWrite)) {
			if (GetLastError() == ERROR_IO_PENDING) {
				GetOverlappedResult(mComFile, &osWrite, &writeByte, true);
				if (writeByte == size)
					return SUCCESS;
				else
					return FAILURE;
			}
		}
		return SUCCESS;
#else
		return FAILURE;
#endif
	}
	RET_CODE serialAdapter::write_sync(unsigned char data)
	{
		if (false == mSerialOpend.load())
			return FAILURE;
#ifdef _WIN32
		return writeData(&data, 1);
#else
		return FAILURE;
#endif
	}
	RET_CODE serialAdapter::write_sync(std::shared_ptr<std::vector<unsigned char>> dat)
	{
		if (false == mSerialOpend.load())
			return FAILURE;
#ifdef _WIN32
		return writeData(dat->data(), dat->size());
#else
		return FAILURE;
#endif
	}
	std::future<bool> serialAdapter::write_async(unsigned char data)
	{
		return std::future<bool>();
	}
	std::future<bool> serialAdapter::write_async(std::shared_ptr<std::vector<unsigned char>> data)
	{
		return std::future<bool>();
	}
}
