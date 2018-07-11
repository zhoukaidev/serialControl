#pragma once

#include <memory>
#include <vector>
#include "SerialListener.h"

namespace scl 
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
	using RET_CODE = ERROR_STATUS;

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
				unsigned long nBaud = SERIAL_LOW_BAUDRATE,
				Parity nParity = Parity::NO_PARITY,
				DataBits nByteSize = DataBits::DATA_BITS_8,
				StopBits stop = StopBits::STOP_BITS_1) = 0;
			virtual RET_CODE close() = 0;
			virtual RET_CODE write_sync(unsigned char data) = 0;
			virtual RET_CODE write_sync(std::shared_ptr<std::vector<unsigned char>> data) = 0;
			virtual RET_CODE write_async(unsigned char data) = 0;
			virtual RET_CODE write_async(std::shared_ptr<std::vector<unsigned char>> data) = 0;
			virtual RET_CODE registerListener(std::weak_ptr<scl::SerialListener> ptr) = 0;
			virtual RET_CODE unRegisterListener(std::weak_ptr<scl::SerialListener> ptr) = 0;
			virtual std::shared_ptr<std::vector<std::string>> enumSerial(void) = 0;
			virtual ~serial(){}
		protected:
			serial(){}
	};


}
