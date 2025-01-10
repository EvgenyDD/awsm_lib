#include <cstdio>
#include <iostream>
#include <string>

// #if !defined(_WIN32) && !defined(__OpenBSD__) && !defined(__FreeBSD__)
// #include <alloca.h>
// #endif

// #if defined (__MINGW32__)
// #define alloca __builtin_alloca
// #endif

// OS Specific sleep
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "dr.h"
#include "serial_interface.h"
#include "serial_interface_list.h"

// int run(int argc, char **argv)
//{
//  // port, baudrate, timeout in milliseconds
//  Serial my_serial(port, baud, Timeout::simpleTimeout(1000));

//  cout << "Is the serial port open?";
//  if(my_serial.isOpen())
//    cout << " Yes." << endl;
//  else
//    cout << " No." << endl;

//  // Get the Test string
//  int count = 0;
//  string test_string;
//  if (argc == 4) {
//    test_string = argv[3];
//  } else {
//    test_string = "Testing.";
//  }

//  // Test the timeout, there should be 1 second between prints
//  cout << "Timeout == 1000ms, asking for 1 more byte than written." << endl;
//  while (count < 10) {
//    size_t bytes_wrote = my_serial.write(test_string);

//    string result = my_serial.read(test_string.length()+1);

//    cout << "Iteration: " << count << ", Bytes written: ";
//    cout << bytes_wrote << ", Bytes read: ";
//    cout << result.length() << ", String read: " << result << endl;

//    count += 1;
//  }

//  // Test the timeout at 250ms
//  my_serial.setTimeout(Timeout::max(), 250, 0, 250, 0);
//  count = 0;
//  cout << "Timeout == 250ms, asking for 1 more byte than written." << endl;
//  while (count < 10) {
//    size_t bytes_wrote = my_serial.write(test_string);

//    string result = my_serial.read(test_string.length()+1);

//    cout << "Iteration: " << count << ", Bytes written: ";
//    cout << bytes_wrote << ", Bytes read: ";
//    cout << result.length() << ", String read: " << result << endl;

//    count += 1;
//  }

//  // Test the timeout at 250ms, but asking exactly for what was written
//  count = 0;
//  cout << "Timeout == 250ms, asking for exactly what was written." << endl;
//  while (count < 10) {
//    size_t bytes_wrote = my_serial.write(test_string);

//    string result = my_serial.read(test_string.length());

//    cout << "Iteration: " << count << ", Bytes written: ";
//    cout << bytes_wrote << ", Bytes read: ";
//    cout << result.length() << ", String read: " << result << endl;

//    count += 1;
//  }

//  // Test the timeout at 250ms, but asking for 1 less than what was written
//  count = 0;
//  cout << "Timeout == 250ms, asking for 1 less than was written." << endl;
//  while (count < 10) {
//    size_t bytes_wrote = my_serial.write(test_string);

//    string result = my_serial.read(test_string.length()-1);

//    cout << "Iteration: " << count << ", Bytes written: ";
//    cout << bytes_wrote << ", Bytes read: ";
//    cout << result.length() << ", String read: " << result << endl;

//    count += 1;
//  }
//}

int main(int argc, char **argv)
{
	// std::vector<PortInfo> devices_found = serial_port_get_list();
	// std::vector<PortInfo>::iterator iter = devices_found.begin();
	// if(!devices_found.size())
	// {
	// 	printf("No COM ports avail!\n");
	// 	return -1;
	// }

	// printf("Avail. COM devices: \n");
	// PortInfo device;
	// while(iter != devices_found.end())
	// {
	// 	device = *iter++;
	// 	printf("\t%s\t%s\t%s\n", device.port.c_str(), device.description.c_str(), device.hardware_id.c_str());
	// }

	serial_port_list_ids_t sp_list;
	serial_port_info_t sp_info;
	serial_port_list_start(sp_list);
	std::string port_to_use = "";
	while(serial_port_list_get_next(sp_list, sp_info))
	{
		printf("SP: %s %s %s\n", sp_info.port, sp_info.hardware_id, sp_info.description);
		/*if(port_to_use == "") */ port_to_use = std::string(sp_info.port);
	}
	serial_port_list_terminate(sp_list);

	serial_port_t p = {0};
	serial_port_init(&p,
					 /*"/dev/ttyS6"*/ /*device.port*/ port_to_use,
					 115200,
					 eightbits,
					 parity_odd,
					 stopbits_one,
					 flowcontrol_none);

	int sts = serial_port_open(&p);
	printf("Open sts: %d\n", sts);
	if(sts) return sts;

	sts = serial_port_write(&p, {9, 0, 1, 2, 3});
	printf("Wr sts: %d\n", sts);

	std::vector<uint8_t> b;
	sts = serial_port_read(&p, &b);
	printf("Rd sts: %d\n", sts);

	printf("Readout: ");
	for(auto i : b)
	{
		printf("%d ", i);
	}
	printf("\n");
}
