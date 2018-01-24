// SerialControl.cpp : Defines the entry point for the console application.
//

#include "SerialListener.h"
#include "SerialAdapter.h"
#include <iostream>


std::condition_variable globalCV;

class DataCollector : public oym::SerialListener
{
public:
	void onConnected()
	{
		std::cout << "[INFO]: Serial connected\n";
	}

	void onDisconnected()
	{
		std::cout << "[INFO]: Serial disconnected\n";
		globalCV.notify_all();
	}

	void onData(unsigned char data)
	{
		std::cout << (int)data << "\n";
	}
};

int main()
{
	std::string serialnum = "com3";
	oym::SerialAdapter adapter(serialnum);
	std::shared_ptr<DataCollector> ptr = std::make_shared<DataCollector>();
	adapter.addListener(ptr);
	while (true) {
		while (true) {
			std::string test;
			std::cout << "[INFO]: Input \"yes\" to open the serial or \"no\" to wait " << serialnum << " \n";
			std::cin >> test;
			if (test == "no")
				continue;
			try {
				adapter.run();
				break;
			}
			catch (const std::exception& e) {
				std::cout << e.what() << std::endl;
			}
		}

		{
			std::mutex mtx;
			std::unique_lock<std::mutex> lck(mtx);
			globalCV.wait(lck);
		}
	}
	system("pause");
	return 0;
}
