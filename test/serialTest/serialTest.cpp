// SerialControl.cpp : Defines the entry point for the console application.
//

#include "SerialListener.h"
#include "SerialAdapter.h"
#include <iostream>
#include <numeric>

std::promise<bool> quitPromise;
class DataCollector : public oym::SerialListener
{
public:

	void onClosed()
	{
		std::cout << "[INFO]: Serial disconnected" << std::endl;
		quitPromise.set_value(true);
	}

	void onData(unsigned char data)
	{
		std::cout << static_cast<unsigned int>(data) << " ";
		if (data == 10 || data == 20)
			std::cout << "\n";
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
	std::this_thread::sleep_for(std::chrono::seconds(2));
	auto fu = quitPromise.get_future();
	while (std::future_status::timeout ==
			fu.wait_for(std::chrono::milliseconds(100))){
		adapter.write_sync(0x01);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		auto prt1 = std::make_shared<std::vector<unsigned char>>(
			std::initializer_list<unsigned char>{2,3,4,5,6,7,8,9,10});
		if (oym::FAILURE == adapter.write_sync(prt1))
			std::cout << "send failure" << std::endl;
		adapter.write_async(11);
		std::iota(prt1->begin(), prt1->end(), 12);
		adapter.write_async(prt1);
	}
	return 0;
}
