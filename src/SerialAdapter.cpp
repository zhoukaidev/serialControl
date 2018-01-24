#include "SerialAdapter.h"
#include <algorithm>
#include <cctype>
#include <atlstr.h>
#include <iostream>
#include <stdexcept>
namespace oym
{

	SerialAdapter::SerialAdapter(std::string& com, DWORD nBaud ,
		unsigned char nParity , unsigned char nByteSize , unsigned char nStopBit ):mSerialNum(com)
	{
		mDcb.BaudRate = nBaud;
		mDcb.Parity = nParity;
		mDcb.ByteSize = nByteSize;
		mDcb.StopBits = nStopBit;
	}
	
	SerialAdapter::~SerialAdapter()
	{
		stop();
	}

	bool SerialAdapter::openSerialPort(void)
	{
		if (mSerialNum.size() < 4) { return false; }
		std::transform(mSerialNum.begin(), mSerialNum.end(),mSerialNum.begin(),::toupper);
		if (mSerialNum.substr(0, 3) != "COM" || std::all_of(mSerialNum.begin() + 3, mSerialNum.end(), std::isdigit) == false) {
			return false;
		}
#ifdef _WIN32
//workaround
		CString sCom;
		std::string comNum = mSerialNum.substr(3, std::string::npos);
		sCom.Format(_T("\\\\.\\COM%d"), std::stoi(comNum,nullptr));
		mComFile= CreateFile(sCom.GetBuffer(50),
			GENERIC_READ | GENERIC_WRITE,
			0,/* do not share*/
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL);
		if (mComFile == INVALID_HANDLE_VALUE) {
			return false;
		}
		if (SetupSerialPort(50) == false) {
			CloseHandle(mComFile);
			return false;
		}
		return true;
#endif
	}

	bool SerialAdapter::SetupSerialPort(unsigned short readTimeout)
	{
#ifdef _WIN32
		DCB ndcb;
		COMMTIMEOUTS timeouts;
		timeouts.ReadIntervalTimeout = 0;
		timeouts.ReadTotalTimeoutMultiplier = 0;
		timeouts.ReadTotalTimeoutConstant = readTimeout;
		timeouts.WriteTotalTimeoutConstant = 0;
		timeouts.WriteTotalTimeoutMultiplier = 0;
		SetCommTimeouts(mComFile, &timeouts);
		if (!GetCommState(mComFile, &ndcb)) {
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

		if (!SetCommState(mComFile, &ndcb)) {
			throw std::runtime_error("SetCommState failed\n");
		}
		
		PurgeComm(mComFile, PURGE_RXCLEAR | PURGE_TXCLEAR);

		/*clear error*/
		DWORD dwError;
		COMSTAT cs;
		if (!ClearCommError(mComFile, &dwError, &cs)) {
			throw std::runtime_error("ClearCommError failed\n");
		}

		/*set mask*/
		SetCommMask(mComFile, EV_RXCHAR);

		return true;
#endif
	}

	void SerialAdapter::addListener(std::shared_ptr<oym::SerialListener> ptr)
	{
		std::lock_guard<std::mutex> lck(mListenerMTX);
		if (std::find(mListeners.begin(), mListeners.end(), ptr) != mListeners.end()) {
			return;
		}
		mListeners.push_back(ptr);
	}

	void SerialAdapter::removeListener(std::shared_ptr<oym::SerialListener> ptr)
	{
		auto I = std::find(mListeners.begin(), mListeners.end(), ptr);
		if (I == mListeners.end()) {
			return;
		}
		mListeners.erase(I);
	}

	bool SerialAdapter::run(void)
	{
		bool ret = openSerialPort();
		if (ret == false) {
			throw std::runtime_error("Unable open the serial:" + mSerialNum);
		}
		{
			std::lock_guard<std::mutex> lck(mListenerMTX);
			for (auto listner : mListeners) {
				listner->onConnected();
			}
		}
		mReadThread = std::thread(std::mem_fun(&oym::SerialAdapter::readSerialData),this);
		mDetectThread = std::thread(std::mem_fun(&oym::SerialAdapter::detectSerialAvaliable), this);
		return true;
	}

	bool SerialAdapter::stop(void)
	{
		//mReadCV.notify_all();
		mReadThreadQuit.store(true);
		if (mReadThread.joinable()) {
			mReadThread.join();
		}
		mDetectThreadQuit.store(true);
		if (mDetectThread.joinable()) {
			mDetectThread.join();
		}
		mReadThreadQuit.store(false);
		mDetectThreadQuit.store(false);
		if (mComFile) {
			CloseHandle(mComFile);
			mComFile = nullptr;
		}
		return true;
	}

	void SerialAdapter::readSerialData(void)
	{
#ifdef _WIN32
		unsigned char data = 0x00;
		DWORD readLen = 0;
		OVERLAPPED osRead;
		memset(&osRead, 0, sizeof(OVERLAPPED));
		osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		bool loop = true;
		//std::mutex mtx;
		while (loop) {
			//std::unique_lock<std::mutex> lck(mtx);
			//if (mReadCV.wait_for(lck, std::chrono::seconds(0)) == std::cv_status::no_timeout) {
			//	return;
			//}
			if (mReadThreadQuit.load() == true) {
				return;
			}
			if (!ReadFile(mComFile, &data, 1, &readLen, &osRead)) {
				if (GetLastError() == ERROR_IO_PENDING) {
					GetOverlappedResult(mComFile, &osRead, &readLen, true);
				}
			}
			if (readLen) {
				readLen = 0;
				std::lock_guard<std::mutex> ltx(mListenerMTX);
				for (auto listen : mListeners) {
					listen->onData(data);
				}
			}
		}
#endif
	}

	bool SerialAdapter::checkPortAvaliable(void)
	{
#ifdef _WIN32
		//workaround
		HANDLE comFile;
		CString sCom;
		std::string comNum = mSerialNum.substr(3, std::string::npos);
		sCom.Format(_T("\\\\.\\COM%d"), std::stoi(comNum, nullptr));
		comFile = CreateFile(sCom.GetBuffer(50),
			GENERIC_READ | GENERIC_WRITE,
			0,/* do not share*/
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL);
		if (comFile == INVALID_HANDLE_VALUE) {
			if (ERROR_ACCESS_DENIED == GetLastError()) {
				return false;
			}
			else if (ERROR_FILE_NOT_FOUND == GetLastError()) {
				return true;
			}
		}
		else {
			CloseHandle(comFile);
			return true;
		}
		return false;
#endif
	}

	void SerialAdapter::detectSerialAvaliable(void)
	{
#ifdef _WIN32
		bool loop = true;
		while (loop) {
			if (mDetectThreadQuit.load() == true) {
				return;
			}
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			if (checkPortAvaliable() == true) {
				{
					std::lock_guard<std::mutex> ltx(mListenerMTX);
					for (auto listener : mListeners) {
						listener->onDisconnected();
					}
				}
				std::thread temp(std::mem_fun(&oym::SerialAdapter::stop),this);
				temp.detach();
				//mReadThreadQuit.store(true);
				//if (mReadThread.joinable()) {
				//	mReadThread.join();
				//}
				//mReadThreadQuit.store(false);
				return;
			}
#endif
		}
	}
}