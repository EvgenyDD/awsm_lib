#ifndef __SERIAL_INTERFACE_LIST_H__
#define __SERIAL_INTERFACE_LIST_H__

#include <string>
#include <vector>

#define PORT_NAME_MAX_LENGTH (256)
#define FRIENDLY_NAME_MAX_LENGTH (256)
#define HARDWARE_ID_MAX_LENGTH (256)

struct PortInfo
{
	std::string port;
	std::string description;
	std::string hardware_id; /// VID:PID of USB serial devices
};

typedef struct
{
	char port[PORT_NAME_MAX_LENGTH];
	char description[FRIENDLY_NAME_MAX_LENGTH];
	char hardware_id[HARDWARE_ID_MAX_LENGTH];
} serial_port_info_t;


#if defined(_WIN32)

#include <tchar.h>

#include <winsock2.h> // before Windows.h, else Winsock 1 conflict

#include <windows.h>

#include <setupapi.h>

typedef struct
{
	HDEVINFO device_info_set;
	unsigned int device_info_set_index;
} serial_port_list_ids_t;
#else

#include <glob.h>

typedef struct
{
	glob_t globbuf;
	size_t iter;
} serial_port_list_ids_t;
#endif

void serial_port_list_start(serial_port_list_ids_t &sp_list);
int serial_port_list_get_next(serial_port_list_ids_t &sp_list, serial_port_info_t &info);
void serial_port_list_terminate(serial_port_list_ids_t &sp_list);

#endif // __SERIAL_INTERFACE_LIST_H__