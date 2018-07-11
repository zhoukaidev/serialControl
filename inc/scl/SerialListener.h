#pragma once

namespace scl 
{
	class SerialListener
	{
	public:
		/// \brief serial port is unplug by manual when serial port at open mode
		/// \param	none 
		/// \ret	none
		virtual void onClosed() = 0;

		/// \brief received data from serial port
		/// \param 	[in]	
		/// \ret	none
		virtual void onData(unsigned char data) = 0;

	protected:
		virtual ~SerialListener() {};
	};

}
