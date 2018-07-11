// SerialControl.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <numeric>
#include <vector>
#include <optional>
#include <future>
#include "scl\serialManager.h"

std::promise<bool> quitPromise;
class DataCollector : public scl::SerialListener
{
public:
	void onClosed()
	{
		std::cout << "[INFO]: Serial disconnected" << std::endl;
		quitPromise.set_value(true);
	}

	void onData(unsigned char data)
	{
		std::cout << data << " ";
	}
};

int main(int argc, char** argv)
{
	auto mser = scl::serialManager::createSerial();
	std::shared_ptr<DataCollector> ptr = std::make_shared<DataCollector>();
	mser->registerListener(ptr);
	unsigned int comport = 0;
	std::cout << "Please input serial number: ";
	std::cin >> comport;
	std::cout << "comport:	"<<	comport << std::endl;
	auto ret = mser->open(comport,115200UL);
	if (ret != scl::SUCCESS) {
		std::cout << "Open Serial Port failure\n";
		return 0;
	}
	else {
		std::cout << "Open serial port success" << std::endl;
	}
	std::this_thread::sleep_for(std::chrono::seconds(2));
	auto fu = quitPromise.get_future();
	while (std::future_status::timeout ==
			fu.wait_for(std::chrono::milliseconds(100))){
		mser->write_sync(0x01);
		mser->write_async(0x02);
	}
	return 0;
}
