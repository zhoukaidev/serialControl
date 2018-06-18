// SerialControl.cpp : Defines the entry point for the console application.
//

#include "SerialListener.h"
#include "SerialAdapter.h"
#include <iostream>


std::condition_variable globalCV;

class DataCollector : public oym::SerialListener
{
public:

	void onClosed()
	{
		std::cout << "[INFO]: Serial disconnected\n";
		globalCV.notify_all();
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
		
	std::mutex mtx;
	std::unique_lock<std::mutex> ulk(mtx);
	globalCV.wait(ulk);
	return 0;
}
