#include "SerialAdapter.h"
#include "scl/serialManager.h"

namespace scl
{
	std::shared_ptr<serial> serialManager::createSerial(void)
	{
		std::shared_ptr<serial> ptr = std::make_shared<serialAdapter>();
		return  ptr;
	}	
}
