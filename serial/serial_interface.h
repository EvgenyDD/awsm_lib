#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H

#include <cstring>
#include <exception>
#include <limits>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#if defined(_WIN32) && !defined(__MINGW32__)
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;			 // NOLINT
typedef unsigned short uint16_t; // NOLINT
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
// intptr_t and friends are defined in crtdefs.h through stdio.h.
#else
#include <stdint.h>
#endif

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <tchar.h>

#include <winsock2.h> // before Windows.h, else Winsock 1 conflict

#include <windows.h>

#include <setupapi.h>

#include <initguid.h>

#include <devguid.h>

#endif

#if defined(__linux__) || defined(__CYGWIN__)
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include <glob.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if !defined(_WIN32)
#include <pthread.h>
#endif

typedef enum
{
	fivebits = 5,
	sixbits = 6,
	sevenbits = 7,
	eightbits = 8
} bytesize_t;

typedef enum
{
	parity_none = 0,
	parity_odd = 1,
	parity_even = 2,
	parity_mark = 3,
	parity_space = 4
} parity_t;

typedef enum
{
	stopbits_one = 1,
	stopbits_two = 2,
	stopbits_one_point_five
} stopbits_t;

typedef enum
{
	flowcontrol_none = 0,
	flowcontrol_software,
	flowcontrol_hardware
} flowcontrol_t;

struct Timeout
{
	static uint32_t max()
	{
		return 10000000;
	}

	static Timeout simpleTimeout(uint32_t timeout)
	{
		return Timeout(max(), timeout, 0, timeout, 0);
	}

	uint32_t inter_byte_timeout;	   /*! Number of milliseconds between bytes received to timeout on. */
	uint32_t read_timeout_constant;	   /*! A constant number of milliseconds to wait after calling read. */
	uint32_t read_timeout_multiplier;  /*! A multiplier against the number of requested bytes to wait after calling read. */
	uint32_t write_timeout_constant;   /*! A constant number of milliseconds to wait after calling write. */
	uint32_t write_timeout_multiplier; /*! A multiplier against the number of requested bytes to wait after calling write.  */

	Timeout(uint32_t inter_byte_timeout_ = 0,
			uint32_t read_timeout_constant_ = 0,
			uint32_t read_timeout_multiplier_ = 0,
			uint32_t write_timeout_constant_ = 0,
			uint32_t write_timeout_multiplier_ = 0)
		: inter_byte_timeout(inter_byte_timeout_),
		  read_timeout_constant(read_timeout_constant_),
		  read_timeout_multiplier(read_timeout_multiplier_),
		  write_timeout_constant(write_timeout_constant_),
		  write_timeout_multiplier(write_timeout_multiplier_)
	{
	}
};

typedef struct
{
	bool is_open_;

	parity_t parity_;
	bytesize_t bytesize_;
	stopbits_t stopbits_;
	flowcontrol_t flowcontrol_;

	Timeout timeout_;
	unsigned long baudrate_; // Baudrate

#if defined(_WIN32)
	std::wstring port_; // Path to the file descriptor
	HANDLE fd_;
	OVERLAPPED fd_overlap_read, fd_overlap_write, fd_overlap_evt;
	BOOL evt_waiting;
	DWORD evt_mask, evt_mask_len;
	DWORD l;

	HANDLE read_mutex;	// Mutex used to lock the read functions
	HANDLE write_mutex; // Mutex used to lock the write functions
#else
	int fd_;		   // The current file descriptor
	std::string port_; // Path to the file descriptor
	uint8_t buf_rx[8 * 1024];
	bool xonxoff_;
	bool rtscts_;

	uint32_t byte_time_ns_; // Nanoseconds to transmit/receive a single byte

	pthread_mutex_t read_mutex;
	pthread_mutex_t write_mutex;
#endif
} serial_port_t;

#if !defined(_WIN32)

class MillisecondTimer
{
public:
	MillisecondTimer(const uint32_t millis);
	int64_t remaining();

private:
	static timespec timespec_now();
	timespec expiry;
};

#endif // !defined(_WIN32)

// std::string containing the address of the serial port, which would be something like 'COM1' on Windows and '/dev/ttyS0' on Linux.
//           uint32_t baudrate = 9600,
//           Timeout timeout = Timeout(),
//           bytesize_t bytesize = eightbits,
//           parity_t parity = parity_none,
//           stopbits_t stopbits = stopbits_one,
//           flowcontrol_t flowcontrol = flowcontrol_none);

#include <functional>

#include "serial_interface.h"

// class SerialInterface : public AbstractInterface
//{
// public:
//     SerialInterface(bool &success_init, int id, uint32_t interfaceTimeoutMs, std::string &&connectId, std::string &&name) :
//         AbstractInterface(
//             id,
//             interfaceTimeoutMs,
//             std::forward<std::string>(connectId),
//             std::forward<std::string>(name))
//     {
//         PRINT_CTOR();

//        {
//            std::unique_lock<std::shared_timed_mutex> lock(if_impl_mutex);
//            p_impl.reset(new AbstractParser(id, if_impl_mutex));
//        }

//        try
//        {
//            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // to stabilize connection
//            serialPort.reset(new serial::Serial(get_connect_id(), 115200, serial::Timeout::simpleTimeout(0)));

//            success_init = true;
//        }
//        catch(const std::exception &exc)
//        {
//            interface_fault_message = exc.what();
//            interface_is_valid = false;
//            success_init = false;

//            PRINT_INFO("[X]\t" << get_fault_message());
//        }

//        if(interface_is_valid)
//        {
//            //            std::this_thread::sleep_for(std::chrono::milliseconds(10));
//            thread_receive.start(std::bind(&SerialInterface::threadReceive, this));

//            //            auto x = serialPort->getTimeout();

//            //            PRINT_MSG("TO: " << x.inter_byte_timeout);
//            //            PRINT_MSG("TO: " << x.read_timeout_constant);
//            //            PRINT_MSG("TO: " << x.write_timeout_constant);
//            //            PRINT_MSG("TO: " << x.read_timeout_multiplier);
//            //            PRINT_MSG("TO: " << x.write_timeout_multiplier);
//            //            serialPort->setTimeout(x);

//            //            std::vector<uint8_t> data;
//            //            data.resize(20);
//            //            uint32_t var = 511;
//            //            data[0] = (uint8_t)var;
//            //            data[1] = (uint8_t)(var >> 8U);
//            //            var = 5000;
//            //            data[2] = (uint8_t)var;
//            //            data[3] = (uint8_t)(var >> 8U);
//            //            tx_data(data);
//        }
//    }

//    virtual ~SerialInterface()
//    {
//        PRINT_DTOR_PREPARE();

//        thread_receive.terminate();

//        {
//            std::unique_lock<std::shared_timed_mutex> lock(if_impl_mutex);
//            p_impl.reset();
//        }

//        if(interface_is_valid)
//            serialPort->close();

//        PRINT_DTOR();
//    }

//    ErrorCodes tx_data(const std::vector<uint8_t> &data)
//    {
//        try
//        {
//            serialPort->write(data);
//        }
//        catch(const std::exception &exc)
//        {
//            interface_fault_message = exc.what();
//            interface_is_valid = false;
//            return ErrorCodes::GenericError;
//        }
//        return ErrorCodes::Ok;
//    }

// private:
//     std::vector<uint8_t> buffer;
//     int threadReceive()
//     {
//         try
//         {
//             serialPort->read(buffer);

//            if(buffer.size())
//            {
//                p_impl->rx_data(buffer);
//            }

//            std::this_thread::sleep_for(std::chrono::milliseconds(2));
//        }
//        catch(const std::exception &exc)
//        {
//            PRINT_ERROR("FAULT@!" << exc.what());
//            interface_fault_message = exc.what();
//            interface_is_valid = false;

//            return 1;
//        }
//        return 0;
//    }

//    std::unique_ptr<serial::Serial> serialPort;
//};

void serial_port_init(serial_port_t *sp,
					  const std::string &port,
					  unsigned long baudrate,
					  bytesize_t bytesize,
					  parity_t parity,
					  stopbits_t stopbits,
					  flowcontrol_t flowcontrol);
int serial_port_deinit(serial_port_t *sp);

int serial_port_open(serial_port_t *sp);
int serial_port_close(serial_port_t *sp);

int serial_port_readLock(serial_port_t *sp);
int serial_port_writeLock(serial_port_t *sp);

int serial_port_readUnlock(serial_port_t *sp);
int serial_port_writeUnlock(serial_port_t *sp);

std::string serial_port_getPort(serial_port_t *sp);

int serial_port_reconfigure(serial_port_t *sp);

void serial_port_setTimeout(serial_port_t *sp, Timeout *timeout);
Timeout serial_port_getTimeout(serial_port_t *sp);

size_t serial_port_write(serial_port_t *sp, const std::vector<uint8_t> data);
size_t serial_port_read(serial_port_t *sp, std::vector<uint8_t> *buffer);

#endif // SERIAL_INTERFACE_H
