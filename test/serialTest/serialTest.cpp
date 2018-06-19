// SerialControl.cpp : Defines the entry point for the console application.
//

#include "SerialListener.h"
#include "SerialAdapter.h"
#include <iostream>

std::promise<bool> quitPromise;
class DataCollector : public oym::SerialListener
{
public:

	void onClosed()
	{
		quitPromise.set_value(true);
		std::cout << "[INFO]: Serial disconnected\n";
	}

	void onData(unsigned char data)
	{
		std::cout << static_cast<unsigned int>(data) << " ";
	}
};

int main(int argc, char** argv)
{
	oym::serialAdapter adapter;
	std::shared_ptr<DataCollector> ptr = std::make_shared<DataCollector>();
	adapter.registerListener(ptr);
	unsigned int comport = 0;
	std::cout << "Please input serial number: ";
	std::cin >> comport;
	std::cout << "comport:	"<<	comport << std::endl;
	auto ret = adapter.open(comport);
	if (ret != oym::SUCCESS) {
		std::cout << "Open Serial Port failure\n";
		return 0;
	}
	auto fu = quitPromise.get_future();
	unsigned char sendData = 0x00;
	while (std::future_status::timeout ==
			fu.wait_for(std::chrono::milliseconds(100))){
		sendData =	(sendData + 1) % 256;
		adapter.write_sync(sendData);
	}
	return 0;
}
