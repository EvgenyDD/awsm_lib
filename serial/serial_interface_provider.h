#ifndef SERIALPORTPROVIDER_H
#define SERIALPORTPROVIDER_H

#include "serial_interface.h"
#include <condition_variable>
#include <memory>

class SerialInterfaceProvider
{
private:
	//    basic_thread thread_discovery;
	//https://github.com/wjwwood/serial

	int threadDiscovery()
	{
		std::vector<PortInfo> devices_found = serial_port_get_list();

#ifdef WIN32
		const std::string compare = "USB\\VID_0483&PID_5740&REV_0200";
#else
		const std::string compare = "USB VID:PID=0483:5740 SNR=301";
#endif

		std::vector<PortInfo>::iterator iter = devices_found.begin();

		while(iter != devices_found.end())
		{
			PortInfo device = *iter++;
			if(device.hardware_id.c_str() == compare)
			{
				//                PRINT_MSG(device.port.c_str() << " " << /*device.description.c_str() << " " <<*/ device.hardware_id.c_str());

				std::string connId = device.port.c_str();

				//                append_interface<SerialInterface, SerialCanParser>(
				//                            900,
				//                            std::string(connId),
				//                            std::string(connId),
				//                            new UsbSerializer());
			}
		}

		return -100000 /* us */;
	}
};

#endif // SERIALPORTPROVIDER_H
