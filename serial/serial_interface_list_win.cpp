#include "serial_interface_list.h"

#ifdef __cplusplus
#define CAST(t, v) (static_cast<t>(v))
#else
#define CAST(t, v) ((t)(v))
#endif

#if defined(_WIN32)

// #include <windows.h>
// //
// #include <Setupapi.h>
// #include <stringapiset.h>

#include <tchar.h>

#include <winsock2.h> // before Windows.h, else Winsock 1 conflict

#include <windows.h>

#include <setupapi.h>

#include <initguid.h>

#include <devguid.h>

static const DWORD port_name_max_length = 256;
static const DWORD friendly_name_max_length = 256;
static const DWORD hardware_id_max_length = 256;

// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], CAST(int, wstr.size()), nullptr, 0, nullptr, nullptr);
	std::string strTo(CAST(size_t, size_needed), 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], CAST(int, wstr.size()), &strTo[0], size_needed, nullptr, nullptr);
	return strTo;
}

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

		TCHAR port_name[port_name_max_length];
		DWORD port_name_length = port_name_max_length;

		LONG return_code = RegQueryValueEx(hkey, _T("PortName"), nullptr, nullptr, (LPBYTE)port_name, &port_name_length);

		RegCloseKey(hkey);

		if(return_code != EXIT_SUCCESS) continue;

		if(port_name_length > 0 && port_name_length <= port_name_max_length)
			port_name[port_name_length - 1] = '\0';
		else
			port_name[0] = '\0';

		if(_tcsstr(port_name, _T("LPT")) != nullptr) continue; // Ignore parallel ports

		// Get port friendly name
		TCHAR friendly_name[friendly_name_max_length];
		DWORD friendly_name_actual_length = 0;

		BOOL got_friendly_name = SetupDiGetDeviceRegistryProperty(
			device_info_set,
			&device_info_data,
			SPDRP_FRIENDLYNAME,
			nullptr,
			(PBYTE)friendly_name,
			friendly_name_max_length,
			&friendly_name_actual_length);

		if(got_friendly_name == true && friendly_name_actual_length > 0)
			friendly_name[friendly_name_actual_length - 1] = '\0';
		else
			friendly_name[0] = '\0';

		// Get hardware ID
		TCHAR hardware_id[hardware_id_max_length];
		DWORD hardware_id_actual_length = 0;

		BOOL got_hardware_id = SetupDiGetDeviceRegistryProperty(
			device_info_set,
			&device_info_data,
			SPDRP_HARDWAREID,
			nullptr,
			(PBYTE)hardware_id,
			hardware_id_max_length,
			&hardware_id_actual_length);

		if(got_hardware_id == true && hardware_id_actual_length > 0)
			hardware_id[hardware_id_actual_length - 1] = '\0';
		else
			hardware_id[0] = '\0';

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
#endif // #if defined(_WIN32)