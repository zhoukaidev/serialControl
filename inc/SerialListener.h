#pragma once

namespace oym
{
	class SerialListener
	{
	public:
		/// \brief serial port is unplug by manual when serial port at open mode
		virtual void onClosed() = 0;

		/// \brief received data by serial port
		/// \param data
		virtual void onData(unsigned char data) = 0;

	protected:
		virtual ~SerialListener() {};
	};

}