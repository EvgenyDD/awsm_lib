#ifndef __SERIAL_INTERFACE_LIST_H__
#define __SERIAL_INTERFACE_LIST_H__

#include <string>
#include <vector>

struct PortInfo
{
	std::string port;
	std::string description;
	std::string hardware_id; /// VID:PID of USB serial devices
};

std::vector<PortInfo> serial_port_get_list(void);

#endif // __SERIAL_INTERFACE_LIST_H__