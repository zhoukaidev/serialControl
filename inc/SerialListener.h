#pragma once

namespace oym
{
	class SerialListener
	{
	public:
		virtual void onConnected(){}
		virtual void onDisconnected() {}
		virtual void onData(unsigned char data){}
		virtual ~SerialListener(){}
	};

}