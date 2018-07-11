// SerialControl.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <numeric>
#include <vector>
#include <optional>
#include <future>
#include "scl\serialManager.h"

class DataProtocl{
public:
	DataProtocl(){
		payIndex = 0x00;
		mType = DataType::SOF;	
	}
struct format{
	unsigned short length;
	unsigned char cmd0;
	unsigned char cmd1;
	std::vector<unsigned char> payload;
};
std::optional<format> parseData(unsigned char dat)
{
	switch(mType){
		case DataType::SOF:
			if(dat == 0xfe){
				mType = DataType::LENGTH_LOW;
				payIndex = 0x00;
				mProData.payload.clear();
			}
		break;
		case DataType::LENGTH_LOW:
			mProData.length = dat;
			mType = DataType::LENGTH_HIGH;
		break;
		case DataType::LENGTH_HIGH:
			mProData.length = ((dat << 8) | mProData.length); 
			mType = DataType::CMD0;
		break;
		case DataType::CMD0:
			mProData.cmd0 = dat;
			mType = DataType::CMD1;	
		break;
		case DataType::CMD1:
			mProData.cmd1 = dat;
			if (mProData.length == 0)
				mType = DataType::FCS;
			else
				mType = DataType::PAYLOAD;
		break;
		case DataType::PAYLOAD:
			mProData.payload.push_back(dat);
			payIndex++;
			if(payIndex ==  mProData.length)
				mType = DataType::FCS;
		break;
		case DataType::FCS:
			if(dat == exclusive_or(mProData)){
				mType = DataType::SOF;
				payIndex = 0x00;
				return std::optional<format>(mProData);
			 } else {
			 	payIndex = 0x00;
				mType = DataType::SOF;
			 }
		break;
		default:
		break;
	}
	return std::nullopt;
}
private:
unsigned char exclusive_or(format& data)
{
	unsigned char res = 0x00;
	res = res ^ (data.length & 0x00ff);
	res = res ^ ((data.length & 0xff00)>>8);
	res = res ^ data.cmd0;
	res = res ^ data.cmd1;
	for(auto& n : data.payload){
		res = res ^ n;
	}
	return res;
}
enum class DataType{
	SOF,
	LENGTH_LOW,
	LENGTH_HIGH,
	CMD0,
	CMD1,
	PAYLOAD,
	FCS
};
DataType mType;
unsigned short payIndex;
format mProData;
};
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
		std::cout << std::hex << static_cast<int>(data) << " ";
		auto ret = mProtoco.parseData(data);
		if(ret){
			auto value = ret.value();
			if (value.payload.size() == 0)
				return;
			if(value.cmd1 == 0x05 && value.payload[0] == 0x02){
				std::cout<<"[INFO]: connection termination event"<<std::endl;
			} else if(value.cmd1 == 0x05 && value.payload[0] == 0x01){
				std::cout<<"[INFO]: connect establishment event"<<std::endl;
			}
			else if (value.cmd1 == 0x05 && value.payload[0] == 0x08) {
				std::cout << "[INFO]: advertising" << std::endl;
			}
			else if (value.cmd1 == 0x89 && value.payload[0] != 00) {
				std::cout << "[ERROR]: Notification reponse error:	"
					<<value.payload[0] << std::endl;
			}
		}
	}
private:
	DataProtocl mProtoco;
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
	}
	return 0;
}
