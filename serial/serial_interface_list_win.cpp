#include "serial_interface_list.h"

#ifdef __cplusplus
#define CAST(t, v) (static_cast<t>(v))
#else
#define CAST(t, v) ((t)(v))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#if defined(_WIN32)

#warning "Windows version"

#include <tchar.h>

#include <winsock2.h> // before Windows.h, else Winsock 1 conflict

#include <windows.h>

#include <setupapi.h>

#include <initguid.h>

#include <devguid.h>

// Convert a wide Unicode string to an UTF8 string
#ifdef UNICODE
static std::string utf8_encode(const std::wstring &wstr)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], CAST(int, wstr.size()), nullptr, 0, nullptr, nullptr);
	std::string strTo(CAST(size_t, size_needed), 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], CAST(int, wstr.size()), &strTo[0], size_needed, nullptr, nullptr);
	return strTo;
}

static void utf8_encode_non_v(const TCHAR *wstr, int wstr_size, char *str, int size_max)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, wstr_size, nullptr, 0, nullptr, nullptr);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_size, str, MAX(size_needed, size_max), nullptr, nullptr);
}
#endif

std::vector<PortInfo> serial_port_get_list(void)
{
	std::vector<PortInfo> devices_found;
	HDEVINFO device_info_set = SetupDiGetClassDevs(CAST(const GUID *, &GUID_DEVCLASS_PORTS), nullptr, nullptr, DIGCF_PRESENT);

	unsigned int device_info_set_index = 0;
	SP_DEVINFO_DATA device_info_data;

	device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

	while(SetupDiEnumDeviceInfo(device_info_set, device_info_set_index, &device_info_data))
	{
		device_info_set_index++;

		// Get port name
		HKEY hkey = SetupDiOpenDevRegKey(device_info_set, &device_info_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

		TCHAR port_name[PORT_NAME_MAX_LENGTH];
		DWORD port_name_length = PORT_NAME_MAX_LENGTH;

		LONG return_code = RegQueryValueEx(hkey, _T("PortName"), nullptr, nullptr, (LPBYTE)port_name, &port_name_length);

		RegCloseKey(hkey);

		if(return_code != EXIT_SUCCESS) continue;

		if(port_name_length > 0 && port_name_length <= PORT_NAME_MAX_LENGTH)
			port_name[port_name_length - 1] = '\0';
		else
			port_name[0] = '\0';

		if(_tcsstr(port_name, _T("LPT")) != nullptr) continue; // Ignore parallel ports

		// Get port friendly name
		TCHAR friendly_name[FRIENDLY_NAME_MAX_LENGTH];
		DWORD friendly_name_length = 0;

		BOOL got_friendly_name = SetupDiGetDeviceRegistryProperty(
			device_info_set,
			&device_info_data,
			SPDRP_FRIENDLYNAME,
			nullptr,
			(PBYTE)friendly_name,
			FRIENDLY_NAME_MAX_LENGTH,
			&friendly_name_length);

		friendly_name[(got_friendly_name == true && friendly_name_length > 0)
						  ? friendly_name_length - 1
						  : 0] = '\0';

		// Get hardware ID
		TCHAR hardware_id[HARDWARE_ID_MAX_LENGTH];
		DWORD hardware_id_length = 0;

		BOOL got_hardware_id = SetupDiGetDeviceRegistryProperty(
			device_info_set,
			&device_info_data,
			SPDRP_HARDWAREID,
			nullptr,
			(PBYTE)hardware_id,
			HARDWARE_ID_MAX_LENGTH,
			&hardware_id_length);

		hardware_id[(got_hardware_id == true && hardware_id_length > 0)
						? hardware_id_length - 1
						: 0] = '\0';

#ifdef UNICODE
		std::string portName = utf8_encode(port_name);
		std::string friendlyName = utf8_encode(friendly_name);
		std::string hardwareId = utf8_encode(hardware_id);
#else
		std::string portName = port_name;
		std::string friendlyName = friendly_name;
		std::string hardwareId = hardware_id;
#endif

		PortInfo port_entry;
		port_entry.port = portName;
		port_entry.description = friendlyName;
		port_entry.hardware_id = hardwareId;
		devices_found.push_back(port_entry);
	}

	SetupDiDestroyDeviceInfoList(device_info_set);

	return devices_found;
}

void serial_port_list_start(serial_port_list_ids_t &list)
{
	list.device_info_set = SetupDiGetClassDevs(CAST(const GUID *, &GUID_DEVCLASS_PORTS), nullptr, nullptr, DIGCF_PRESENT);
	list.device_info_set_index = 0;
}

int serial_port_list_get_next(serial_port_list_ids_t &list, serial_port_info_t &info)
{
	SP_DEVINFO_DATA device_info_data;
	device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

	while(SetupDiEnumDeviceInfo(list.device_info_set, list.device_info_set_index, &device_info_data))
	{
		list.device_info_set_index++;

		// Get port name
		HKEY hkey = SetupDiOpenDevRegKey(list.device_info_set, &device_info_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

		TCHAR port_name[PORT_NAME_MAX_LENGTH];
		DWORD port_name_length = PORT_NAME_MAX_LENGTH;
		LONG return_code = RegQueryValueEx(hkey, _T("PortName"), nullptr, nullptr, (LPBYTE)port_name, &port_name_length);

		RegCloseKey(hkey);

		if(return_code != EXIT_SUCCESS) continue;

		port_name_length = (port_name_length > 0 && port_name_length <= PORT_NAME_MAX_LENGTH) ? port_name_length : 0;
		port_name[port_name_length ? port_name_length - 1 : 0] = '\0';

		if(_tcsstr(port_name, _T("LPT")) != nullptr) continue; // Ignore parallel ports

		TCHAR friendly_name[FRIENDLY_NAME_MAX_LENGTH];
		DWORD friendly_name_length = 0;

		BOOL got_friendly_name = SetupDiGetDeviceRegistryProperty(
			list.device_info_set,
			&device_info_data,
			SPDRP_FRIENDLYNAME,
			nullptr,
			(PBYTE)friendly_name,
			FRIENDLY_NAME_MAX_LENGTH,
			&friendly_name_length);

		friendly_name_length = got_friendly_name == true && friendly_name_length > 0 ? friendly_name_length : 0;
		friendly_name[friendly_name_length ? friendly_name_length - 1 : 0] = '\0';

		TCHAR hardware_id[HARDWARE_ID_MAX_LENGTH];
		DWORD hardware_id_length = 0;

		BOOL got_hardware_id = SetupDiGetDeviceRegistryProperty(
			list.device_info_set,
			&device_info_data,
			SPDRP_HARDWAREID,
			nullptr,
			(PBYTE)hardware_id,
			HARDWARE_ID_MAX_LENGTH,
			&hardware_id_length);

		hardware_id_length = (got_hardware_id == true && hardware_id_length > 0) ? hardware_id_length : 0;
		hardware_id[hardware_id_length ? hardware_id_length - 1 : 0] = '\0';

#ifdef UNICODE
		utf8_encode_non_v(port_name, port_name_length, info.port, sizeof(info.port));
		utf8_encode_non_v(friendly_name, friendly_name_length, info.description, sizeof(info.description));
		utf8_encode_non_v(hardware_id, hardware_id_length, info.hardware_id, sizeof(info.hardware_id));
#else
		memcpy(info.port, port_name, MIN(port_name_length, sizeof(info.port)));
		memcpy(info.description, friendly_name, MIN(friendly_name_length, sizeof(info.description)));
		memcpy(info.hardware_id, hardware_id, MIN(hardware_id_length, sizeof(info.hardware_id)));
#endif
		return 1;
	}

	return 0;
}

void serial_port_list_terminate(serial_port_list_ids_t &list)
{
	SetupDiDestroyDeviceInfoList(list.device_info_set);
}

#endif // #if defined(_WIN32)