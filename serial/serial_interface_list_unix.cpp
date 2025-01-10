#include "serial_interface_list.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdarg.h>

#ifdef __cplusplus
#define CAST(t, v) (static_cast<t>(v))
#else
#define CAST(t, v) ((t)(v))
#endif

#if defined(__linux__) || defined(__CYGWIN__)

#warning "UNIX version"

static std::vector<std::string> glob_(const std::vector<std::string> &patterns)
{
	std::vector<std::string> paths_found;
	if(patterns.size() == 0) return paths_found;

	glob_t glob_results;
	/*int glob_retval =*/glob(patterns[0].c_str(), 0, nullptr, &glob_results);

	std::vector<std::string>::const_iterator iter = patterns.begin();
	while(++iter != patterns.end())
	{
		/*glob_retval =*/glob(iter->c_str(), GLOB_APPEND, nullptr, &glob_results);
	}

	for(int path_index = 0; path_index < CAST(int, glob_results.gl_pathc); path_index++)
	{
		paths_found.push_back(glob_results.gl_pathv[path_index]);
	}
	globfree(&glob_results);
	return paths_found;
}

static std::string basename(const std::string &path)
{
	size_t pos = path.rfind("/");
	if(pos == std::string::npos) return path;
	return std::string(path, pos + 1, std::string::npos);
}

static std::string dirname(const std::string &path)
{
	size_t pos = path.rfind("/");
	if(pos == std::string::npos) return path;
	if(pos == 0) return "/";
	return std::string(path, 0, pos);
}

static bool path_exists(const std::string &path)
{
	struct stat sb;
	return stat(path.c_str(), &sb) == 0;
}

static std::string realpath_(const std::string &path)
{
	char *real_path = realpath(path.c_str(), nullptr);
	std::string result;
	if(real_path != nullptr)
	{
		result = real_path;
		free(real_path);
	}
	return result;
}

static std::string format(const char *format, ...)
{
	va_list ap;
	size_t buffer_size_bytes = 256;
	std::string result;
	char *buffer = CAST(char *, malloc(buffer_size_bytes));

	if(buffer == nullptr) return result;

	bool done = false;
	unsigned int loop_count = 0;

	while(!done)
	{
		va_start(ap, format);

		int return_value = vsnprintf(buffer, buffer_size_bytes, format, ap);

		if(return_value < 0)
		{
			done = true;
		}
		else if(return_value >= CAST(int, buffer_size_bytes))
		{
			// Realloc and try again.
			buffer_size_bytes = CAST(size_t, return_value + 1);
			char *new_buffer_ptr = CAST(char *, realloc(buffer, buffer_size_bytes));

			if(new_buffer_ptr == nullptr)
			{
				done = true;
			}
			else
			{
				buffer = new_buffer_ptr;
			}
		}
		else
		{
			result = buffer;
			done = true;
		}

		va_end(ap);

		if(++loop_count > 5) done = true;
	}
	free(buffer);
	return result;
}

static std::string read_line(const std::string &file)
{
	std::ifstream ifs(file.c_str(), std::ifstream::in);
	std::string line;
	if(ifs)
	{
		getline(ifs, line);
	}
	return line;
}

static std::string usb_sysfs_friendly_name(const std::string &sys_usb_path)
{
	unsigned int device_number = 0;
	std::istringstream(read_line(sys_usb_path + "/devnum")) >> device_number;
	std::string manufacturer = read_line(sys_usb_path + "/manufacturer");
	std::string product = read_line(sys_usb_path + "/product");
	std::string serial = read_line(sys_usb_path + "/serial");
	if(manufacturer.empty() && product.empty() && serial.empty()) return "";
	return format("%s %s %s", manufacturer.c_str(), product.c_str(), serial.c_str());
}

static std::string usb_sysfs_hw_string(const std::string &sysfs_path)
{
	std::string serial_number = read_line(sysfs_path + "/serial");

	if(serial_number.length() > 0)
	{
		serial_number = format("SNR=%s", serial_number.c_str());
	}

	std::string vid = read_line(sysfs_path + "/idVendor");
	std::string pid = read_line(sysfs_path + "/idProduct");

	return format("USB VID:PID=%s:%s %s", vid.c_str(), pid.c_str(), serial_number.c_str());
}

static std::vector<std::string> get_sysfs_info_(const std::string &device_path)
{
	std::string device_name = basename(device_path);
	std::string friendly_name;
	std::string hardware_id;
	std::string sys_device_path = format("/sys/class/tty/%s/device", device_name.c_str());

	if(device_name.compare(0, 6, "ttyUSB") == 0)
	{
		sys_device_path = dirname(dirname(realpath_(sys_device_path)));

		if(path_exists(sys_device_path))
		{
			friendly_name = usb_sysfs_friendly_name(sys_device_path);
			hardware_id = usb_sysfs_hw_string(sys_device_path);
		}
	}
	else if(device_name.compare(0, 6, "ttyACM") == 0)
	{
		sys_device_path = dirname(realpath_(sys_device_path));

		if(path_exists(sys_device_path))
		{
			friendly_name = usb_sysfs_friendly_name(sys_device_path);
			hardware_id = usb_sysfs_hw_string(sys_device_path);
		}
	}
	else
	{
		// Try to read ID string of PCI device
		std::string sys_id_path = sys_device_path + "/id";
		if(path_exists(sys_id_path)) hardware_id = read_line(sys_id_path);
	}

	if(friendly_name.empty()) friendly_name = device_name;
	if(hardware_id.empty()) hardware_id = "n/a";

	std::vector<std::string> result;
	result.push_back(friendly_name);
	result.push_back(hardware_id);
	return result;
}

std::vector<PortInfo> serial_port_get_list(void)
{
	std::vector<PortInfo> results;

	std::vector<std::string> search_globs;
	search_globs.push_back("/dev/ttyACM*");
	search_globs.push_back("/dev/ttyS*");
	search_globs.push_back("/dev/ttyUSB*");
	search_globs.push_back("/dev/tty.*");
	search_globs.push_back("/dev/cu.*");

	std::vector<std::string> devices_found = glob_(search_globs);

	std::vector<std::string>::iterator iter = devices_found.begin();

	while(iter != devices_found.end())
	{
		std::string device = *iter++;
		std::vector<std::string> sysfs_info = get_sysfs_info_(device);
		std::string friendly_name = sysfs_info[0];
		std::string hardware_id = sysfs_info[1];

		PortInfo device_entry;
		device_entry.port = device;
		device_entry.description = friendly_name;
		device_entry.hardware_id = hardware_id;
		results.push_back(device_entry);
	}
	return results;
}

static const char *search_sources[] = {
	"/dev/ttyACM*",
	"/dev/ttyS*",
	"/dev/ttyUSB*",
	"/dev/tty.*",
	"/dev/cu.*",
};

void serial_port_list_start(serial_port_list_ids_t &list)
{
	glob(search_sources[0], 0, nullptr, &list.globbuf);
	for(size_t i = 1; i < sizeof(search_sources) / sizeof(search_sources[0]); i++)
	{
		glob(search_sources[i], GLOB_APPEND, nullptr, &list.globbuf);
	}
	list.iter = 0;

	// for(int path_index = 0; path_index < CAST(int, list.globbuf.gl_pathc); path_index++)
	// {
	// 	paths_found.push_back(list.globbuf.gl_pathv[path_index]);
	// }

	printf("SOURCES Count: %ld\n", list.globbuf.gl_pathc);
}

int serial_port_list_get_next(serial_port_list_ids_t &list, serial_port_info_t &info)
{
	while(list.iter < list.globbuf.gl_pathc)
	{
		std::string device = *iter++;

		{
			size_t pos = path.rfind("/");
			if(pos == std::string::npos) return path;
			return std::string(path, pos + 1, std::string::npos);
		}
		std::string device_name = basename(device_path);
		std::string friendly_name;
		std::string hardware_id;
		std::string sys_device_path = format("/sys/class/tty/%s/device", device_name.c_str());

		if(device_name.compare(0, 6, "ttyUSB") == 0)
		{
			sys_device_path = dirname(dirname(realpath_(sys_device_path)));

			if(path_exists(sys_device_path))
			{
				friendly_name = usb_sysfs_friendly_name(sys_device_path);
				hardware_id = usb_sysfs_hw_string(sys_device_path);
			}
		}
		else if(device_name.compare(0, 6, "ttyACM") == 0)
		{
			sys_device_path = dirname(realpath_(sys_device_path));

			if(path_exists(sys_device_path))
			{
				friendly_name = usb_sysfs_friendly_name(sys_device_path);
				hardware_id = usb_sysfs_hw_string(sys_device_path);
			}
		}
		else
		{
			// Try to read ID string of PCI device
			std::string sys_id_path = sys_device_path + "/id";
			if(path_exists(sys_id_path)) hardware_id = read_line(sys_id_path);
		}

		if(friendly_name.empty()) friendly_name = device_name;
		if(hardware_id.empty()) hardware_id = "n/a";

		std::vector<std::string> result;
		result.push_back(friendly_name);
		result.push_back(hardware_id);

		info.port = device;
		info.description = friendly_name;
		info.hardware_id = hardware_id;

		list.iter++;
		return 1;
	}
	return 0;
}

void serial_port_list_terminate(serial_port_list_ids_t &list)
{
	globfree(&list.globbuf);
}

#endif // defined(__linux__)