# serialControl SDK

## Brief
serialControl SDK is the software development kit to access serial port

**Note**:
> So far, only Windows is supported.

## Build and run the example code
The following steps instruct you to building and running the example code:

1. Open `script/msvc/serialControl.sln` in VisualStudio 2017. So far, the solution includes two projects which 
is named `serialTest` and `serialSDK`.

2. Build the project `serialTest`, and the executable will be generated in the `bin` directory and the 
static library in the `lib` directory.

3. Run the executable file of previously built `serialTest`, different messages will be display depends on if the serial device is connected or `com port` is avaliable.

## Sample code

```c++
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
    // create the serial port listener
	std::shared_ptr<DataCollector> ptr = std::make_shared<DataCollector>();
    //  register serial port listener
	adapter.registerListener(ptr);
	unsigned int comport = 0;
	std::cout << "Please input serial number: ";
	std::cin >> comport;
	std::cout << "comport:	"<<	comport << std::endl;
    // open serial port and start monitor mode
	auto ret = adapter.open(comport);
	if (ret != oym::SUCCESS) {
		std::cout << "Open Serial Port failure\n";
		return 0;
	}
		
	std::mutex mtx;
	std::unique_lock<std::mutex> ulk(mtx);
    // if serial device is pulled out from the serial port,just exit
	globalCV.wait(ulk);
	return 0;
}
```
## Known issues
1. [serialControl][serialControl] is in early version,the public API is unstable.

[serialControl]: https://www.github.com/zhoukaisspu/serialControl
