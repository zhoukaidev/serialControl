#pragma once
#include "serial.h"
#include <memory>
namespace scl
{
	class serialManager{
	public:
		static std::shared_ptr<serial> createSerial(void);
	protected:
		serialManager(){}	
		~serialManager(){}
	};
}
