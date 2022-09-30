#include "serial_interface.h"
#include <chrono>
#include <thread>

#ifdef __cplusplus
#define CAST(t, v) (static_cast<t>(v))
#else
#define CAST(t, v) ((t)(v))
#endif

#if defined(_WIN32)

#include <sstream>
#include <stdio.h>

#define SERIAL_OVERLAP_MODE

static std::wstring _prefix_port_if_needed(const std::wstring *input)
{
	static std::wstring windows_com_port_prefix = L"\\\\.\\";
	if(input->compare(windows_com_port_prefix) != 0) return windows_com_port_prefix + *input;
	return *input;
}

void serial_port_setTimeout(serial_port_t *sp, Timeout *timeout)
{
	sp->timeout_ = *timeout;
	if(sp->is_open_)
	{
		serial_port_reconfigure(sp);
	}
}

Timeout serial_port_getTimeout(serial_port_t *sp) { return sp->timeout_; }

void serial_port_setBaudrate(serial_port_t *sp, unsigned long baudrate)
{
	sp->baudrate_ = baudrate;
	if(sp->is_open_)
	{
		serial_port_reconfigure(sp);
	}
}

unsigned long serial_port_getBaudrate(serial_port_t *sp) { return sp->baudrate_; }

void serial_port_setBytesize(serial_port_t *sp, bytesize_t bytesize)
{
	sp->bytesize_ = bytesize;
	if(sp->is_open_)
	{
		serial_port_reconfigure(sp);
	}
}

bytesize_t serial_port_getBytesize(serial_port_t *sp) { return sp->bytesize_; }

void serial_port_setParity(serial_port_t *sp, parity_t parity)
{
	sp->parity_ = parity;
	if(sp->is_open_)
	{
		serial_port_reconfigure(sp);
	}
}

parity_t serial_port_getParity(serial_port_t *sp) { return sp->parity_; }

void serial_port_setStopbits(serial_port_t *sp, stopbits_t stopbits)
{
	sp->stopbits_ = stopbits;
	if(sp->is_open_)
	{
		serial_port_reconfigure(sp);
	}
}

stopbits_t serial_port_getStopbits(serial_port_t *sp) { return sp->stopbits_; }

void serial_port_setFlowcontrol(serial_port_t *sp, flowcontrol_t flowcontrol)
{
	sp->flowcontrol_ = flowcontrol;
	if(sp->is_open_)
	{
		serial_port_reconfigure(sp);
	}
}

flowcontrol_t serial_port_getFlowcontrol(serial_port_t *sp) { return sp->flowcontrol_; }

void serial_port_init(serial_port_t *sp,
					  const std::string &port,
					  unsigned long baudrate,
					  bytesize_t bytesize,
					  parity_t parity,
					  stopbits_t stopbits,
					  flowcontrol_t flowcontrol)
{
	sp->port_ = std::wstring(port.begin(), port.end());
	sp->fd_ = INVALID_HANDLE_VALUE;
	sp->is_open_ = false;
	sp->baudrate_ = baudrate;
	sp->parity_ = parity;
	sp->bytesize_ = bytesize;
	sp->stopbits_ = stopbits;
	sp->flowcontrol_ = flowcontrol;
	sp->read_mutex = CreateMutex(nullptr, false, nullptr);
	sp->write_mutex = CreateMutex(nullptr, false, nullptr);
}

void seral_port_deinit(serial_port_t *sp)
{
	serial_port_close(sp);
	CloseHandle(sp->read_mutex);
	CloseHandle(sp->write_mutex);
}

int serial_port_open(serial_port_t *sp)
{
	if(sp->port_.empty())
	{
		printf("Empty port is invalid.\n");
		return 1;
	}
	if(sp->is_open_ == true)
	{
		printf("Serial port already open.\n");
		return 2;
	}

	// See: https://github.com/wjwwood/serial/issues/84
	std::wstring port_with_prefix = _prefix_port_if_needed(&sp->port_);
	LPCWSTR lp_port = port_with_prefix.c_str();
#ifdef SERIAL_OVERLAP_MODE
	sp->fd_ = CreateFileW(lp_port,
						  GENERIC_READ | GENERIC_WRITE,
						  0,
						  nullptr,
						  OPEN_EXISTING,
						  FILE_FLAG_OVERLAPPED,
						  nullptr);
#else
	sp->fd_ = CreateFileW(lp_port,
						  GENERIC_READ | GENERIC_WRITE,
						  0,
						  nullptr,
						  OPEN_EXISTING,
						  FILE_ATTRIBUTE_NORMAL,
						  nullptr);
#endif

	if(sp->fd_ == INVALID_HANDLE_VALUE)
	{
		DWORD create_file_err = GetLastError();
		switch(create_file_err)
		{
		case ERROR_FILE_NOT_FOUND:
			printf("[E]\tSpecified port '%s' does not exist", serial_port_getPort(sp).c_str());
			return 3;

		case ERROR_ACCESS_DENIED:
			printf("[E]\tAccess denied when opening '%s' serial port", serial_port_getPort(sp).c_str());
			return 4;

		default:
			printf("[E]\tUnknown error (%ld) opening the '%s' serial port", create_file_err, serial_port_getPort(sp).c_str());
			return 5;
		}
	}

	serial_port_reconfigure(sp);
	sp->is_open_ = true;
	return 0;
}

int serial_port_reconfigure(serial_port_t *sp)
{
	if(sp->fd_ == INVALID_HANDLE_VALUE) return -11;

	DCB dcbSerialParams;
	memset(&dcbSerialParams, 0, sizeof(DCB));
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if(!GetCommState(sp->fd_, &dcbSerialParams))
	{
		printf("[E]\tError getting the serial port state\n");
		return -2;
	}

	// setup baud rate
	switch(sp->baudrate_)
	{
	case 110: dcbSerialParams.BaudRate = CBR_110; break;
	case 300: dcbSerialParams.BaudRate = CBR_300; break;
	case 600: dcbSerialParams.BaudRate = CBR_600; break;
	case 1200: dcbSerialParams.BaudRate = CBR_1200; break;
	case 2400: dcbSerialParams.BaudRate = CBR_2400; break;
	case 4800: dcbSerialParams.BaudRate = CBR_4800; break;
	case 9600: dcbSerialParams.BaudRate = CBR_9600; break;
	case 14400: dcbSerialParams.BaudRate = CBR_14400; break;
	case 19200: dcbSerialParams.BaudRate = CBR_19200; break;
	case 57600: dcbSerialParams.BaudRate = CBR_57600; break;
	case 38400: dcbSerialParams.BaudRate = CBR_38400; break;
	case 115200: dcbSerialParams.BaudRate = CBR_115200; break;
	case 128000: dcbSerialParams.BaudRate = CBR_128000; break;
	case 256000: dcbSerialParams.BaudRate = CBR_256000; break;
	default: dcbSerialParams.BaudRate = sp->baudrate_; break;
	}
	switch(sp->bytesize_)
	{
	case fivebits: dcbSerialParams.ByteSize = 5; break;
	case sixbits: dcbSerialParams.ByteSize = 6; break;
	case sevenbits: dcbSerialParams.ByteSize = 7; break;
	default:
	case eightbits: dcbSerialParams.ByteSize = 8; break;
	}

	switch(sp->stopbits_)
	{
	case stopbits_two: break;
	case stopbits_one_point_five: dcbSerialParams.StopBits = ONE5STOPBITS; break;
	default:
	case stopbits_one: dcbSerialParams.StopBits = ONESTOPBIT; break;
	}

	switch(sp->parity_)
	{
	case parity_even: dcbSerialParams.Parity = EVENPARITY; break;
	case parity_odd: dcbSerialParams.Parity = ODDPARITY; break;
	case parity_mark: dcbSerialParams.Parity = MARKPARITY; break;
	case parity_space: dcbSerialParams.Parity = SPACEPARITY; break;
	default:
	case parity_none: dcbSerialParams.Parity = NOPARITY; break;
	}

	switch(sp->flowcontrol_)
	{
	case flowcontrol_software:
		dcbSerialParams.fOutxCtsFlow = false;
		dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
		dcbSerialParams.fOutX = true;
		dcbSerialParams.fInX = true;
		break;
	case flowcontrol_hardware:
		dcbSerialParams.fOutxCtsFlow = true;
		dcbSerialParams.fRtsControl = RTS_CONTROL_HANDSHAKE;
		dcbSerialParams.fOutX = false;
		dcbSerialParams.fInX = false;
		break;
	default:
	case flowcontrol_none:
		dcbSerialParams.fOutxCtsFlow = false;
		dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
		dcbSerialParams.fOutX = false;
		dcbSerialParams.fInX = false;
		break;
	}

	if(!SetCommState(sp->fd_, &dcbSerialParams))
	{
		CloseHandle(sp->fd_);
		printf("[E]\tError setting serial port settings\n");
		return 3;
	}

	// Setup timeouts
	COMMTIMEOUTS timeouts;
	memset(&timeouts, 0, sizeof(COMMTIMEOUTS));
	timeouts.ReadIntervalTimeout = sp->timeout_.inter_byte_timeout;
	timeouts.ReadTotalTimeoutConstant = sp->timeout_.read_timeout_constant;
	timeouts.ReadTotalTimeoutMultiplier = sp->timeout_.read_timeout_multiplier;
	timeouts.WriteTotalTimeoutConstant = sp->timeout_.write_timeout_constant;
	timeouts.WriteTotalTimeoutMultiplier = sp->timeout_.write_timeout_multiplier;

#ifdef SERIAL_OVERLAP_MODE
	timeouts.ReadIntervalTimeout = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
#endif

	if(!SetCommTimeouts(sp->fd_, &timeouts))
	{
		printf("[E]\tError setting timeouts\n");
		return 4;
	}

#ifdef SERIAL_OVERLAP_MODE
	if(!SetCommMask(sp->fd_, EV_RXCHAR))
	{
		printf("[E]\tError setting CommMask\n");
		return 5;
	}

	memset(&sp->fd_overlap_read, 0, sizeof(OVERLAPPED));
	memset(&sp->fd_overlap_write, 0, sizeof(OVERLAPPED));
	memset(&sp->fd_overlap_evt, 0, sizeof(OVERLAPPED));

	sp->fd_overlap_read.hEvent = CreateEvent(nullptr, true, false, nullptr);
	sp->fd_overlap_write.hEvent = CreateEvent(nullptr, true, false, nullptr);
	sp->fd_overlap_evt.hEvent = CreateEvent(nullptr, true, false, nullptr);
	if(!sp->fd_overlap_read.hEvent || !sp->fd_overlap_write.hEvent || !sp->fd_overlap_evt.hEvent)
	{
		printf("[E]\tError creating overlap objects\n");
		return 6;
	}

	PurgeComm(sp->fd_, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_TXABORT | PURGE_RXABORT);
#endif
	return 0;
}

int serial_port_close(serial_port_t *sp)
{
	if(sp->is_open_ == true)
	{
		if(sp->fd_ != INVALID_HANDLE_VALUE)
		{
			if(CloseHandle(sp->fd_) == 0)
			{
				printf("[E]\tError while closing serial port, %ld\n", GetLastError());
				return 1;
			}
			sp->fd_ = INVALID_HANDLE_VALUE;
		}
		sp->is_open_ = false;
	}
	return 0;
}

size_t serial_port_available(serial_port_t *sp)
{
	if(!sp->is_open_) return 0;

	COMSTAT cs;
	if(!ClearCommError(sp->fd_, nullptr, &cs))
	{
		printf("[E]\tError while checking status of the serial port, %ld\n", GetLastError());
	}
	return CAST(size_t, cs.cbInQue);
}

static size_t _serial_port_read(serial_port_t *sp, std::vector<uint8_t> *buf)
{
	if(!sp->is_open_) return 0;

#ifdef SERIAL_OVERLAP_MODE
	DWORD bytes_to_read = 0;
	DWORD err;
	COMSTAT stat;

	if(!sp->evt_waiting)
	{
		sp->evt_mask = EV_RXCHAR;
		if(!WaitCommEvent(sp->fd_, &sp->evt_mask, &sp->fd_overlap_evt))
		{
			if(GetLastError() != ERROR_IO_PENDING)
			{
				printf("[E]WaitCommEvent failed\n");
				return 0;
			}
			sp->evt_waiting = true;
		}
		else
		{
			ClearCommError(sp->fd_, &err, &stat);
			bytes_to_read = stat.cbInQue;
			sp->evt_waiting = false;
		}
	}

	if(sp->evt_waiting)
	{
		switch(WaitForSingleObject(sp->fd_overlap_evt.hEvent, 10 /* ms */))
		{
		case WAIT_OBJECT_0:
			GetOverlappedResult(sp->fd_, &sp->fd_overlap_evt, &sp->evt_mask_len, false);
			ResetEvent(sp->fd_overlap_evt.hEvent);
			ClearCommError(sp->fd_, &err, &stat);
			bytes_to_read = stat.cbInQue;
			sp->evt_waiting = false;
			break;

		case WAIT_TIMEOUT: return 0;

		default:
			printf("[E]\tGetOverlappedResult failed\n");
			sp->evt_waiting = false;
			return 0;
		}
	}

	if(bytes_to_read)
	{
		buf->resize(bytes_to_read);
		if(!ReadFile(sp->fd_, buf->data(), bytes_to_read, &sp->l, &sp->fd_overlap_read))
		{
			printf("[E]ReadFile failed to read buffered data, %ld\n", GetLastError());
			return 0;
		}
		if(sp->l != bytes_to_read)
		{
			printf("[E]\tRead file length mismatch\n");
		}
		return bytes_to_read;
	}
	return 0;
#else
	DWORD bytes_read;
	size_t av = serial_port_available(sp);
	buf->resize(av);
	if(!ReadFile(sp->fd_, buf->data(), CAST(DWORD, av), &bytes_read, nullptr))
	{
		printf("[E]ReadFile failed to read buffered data, %s\n", GetLastError());
		return 0;
	}
	return bytes_read;
#endif
}

static size_t _serial_port_write(serial_port_t *sp, const uint8_t *data, size_t length)
{
	if(sp->is_open_ == false) return 0;
	DWORD bytes_written;
#ifdef SERIAL_OVERLAP_MODE
	if(!WriteFile(sp->fd_, data, static_cast<DWORD>(length), &bytes_written, &sp->fd_overlap_write))
	{
		switch(WaitForSingleObject(sp->fd_overlap_write.hEvent, INFINITE))
		{
		case WAIT_OBJECT_0:
			GetOverlappedResult(sp->fd_, &sp->fd_overlap_write, &bytes_written, false);
			ResetEvent(&sp->fd_overlap_write.hEvent);
			break;

		default:
		case WAIT_TIMEOUT:
			printf("[E]WriteFile failed, %ld\n", GetLastError());
			return 0;
		}
	}
#else
	if(!WriteFile(sp->fd_, data, static_cast<DWORD>(length), &bytes_written, nullptr)) return 0;
#endif
	return bytes_written;
}

static void _serial_port_setPort(serial_port_t *sp, const std::string *port) { sp->port_ = std::wstring(port->begin(), port->end()); }

std::string serial_port_getPort(serial_port_t *sp) { return std::string(sp->port_.begin(), sp->port_.end()); }

static void _serial_port_flush(serial_port_t *sp)
{
	if(sp->is_open_) FlushFileBuffers(sp->fd_);
}

static void _serial_port_flushInput(serial_port_t *sp)
{
	if(sp->is_open_) PurgeComm(sp->fd_, PURGE_RXCLEAR);
}

static void _serial_port_flushOutput(serial_port_t *sp)
{
	if(sp->is_open_) PurgeComm(sp->fd_, PURGE_TXCLEAR);
}

void serial_port_setBreak(serial_port_t *sp, bool level)
{
	if(sp->is_open_ == false) return;
	EscapeCommFunction(sp->fd_, level ? SETBREAK : CLRBREAK);
}

void serial_port_setRTS(serial_port_t *sp, bool level)
{
	if(sp->is_open_ == false) return;
	EscapeCommFunction(sp->fd_, level ? SETRTS : CLRRTS);
}

void serial_port_setDTR(serial_port_t *sp, bool level)
{
	if(sp->is_open_ == false) return;
	EscapeCommFunction(sp->fd_, level ? SETDTR : CLRDTR);
}

bool serial_port_waitForChange(serial_port_t *sp)
{
	if(sp->is_open_ == false) return false;
	DWORD dwCommEvent;

	if(!SetCommMask(sp->fd_, EV_CTS | EV_DSR | EV_RING | EV_RLSD))
	{
		printf("[E]\tError setting communications mask\n");
		return false;
	}

	if(!WaitCommEvent(sp->fd_, &dwCommEvent, nullptr))
	{
		printf("[E]\tAn error occurred waiting for the event\n");
		return false;
	}
	return true;
}

bool serial_port_getDSR(serial_port_t *sp)
{
	if(sp->is_open_ == false) return false;
	DWORD dwModemStatus;
	if(!GetCommModemStatus(sp->fd_, &dwModemStatus))
	{
		printf("[E]\tError getting the status of the DSR line\n");
		return false;
	}
	return (MS_DSR_ON & dwModemStatus) != 0;
}

bool serial_port_getRI(serial_port_t *sp)
{
	if(sp->is_open_ == false) return false;
	DWORD dwModemStatus;
	if(!GetCommModemStatus(sp->fd_, &dwModemStatus))
	{
		printf("[E]\tError getting the status of the RI line\n");
		return false;
	}
	return (MS_RING_ON & dwModemStatus) != 0;
}

bool serial_port_getCD(serial_port_t *sp)
{
	if(sp->is_open_ == false) return false;
	DWORD dwModemStatus;
	if(!GetCommModemStatus(sp->fd_, &dwModemStatus))
	{
		printf("[E]\tError getting the status of the CD line\n");
	}
	return (MS_RLSD_ON & dwModemStatus) != 0;
}

int serial_port_readLock(serial_port_t *sp)
{
	if(WaitForSingleObject(sp->read_mutex, INFINITE) != WAIT_OBJECT_0)
	{
		printf("[E]\tError claiming read mutex\n");
		return 1;
	}
	return 0;
}

int serial_port_readUnlock(serial_port_t *sp)
{
	if(!ReleaseMutex(sp->read_mutex))
	{
		printf("[E]\tError releasing read mutex\n");
		return 1;
	}
	return 0;
}

int serial_port_writeLock(serial_port_t *sp)
{
	if(WaitForSingleObject(sp->write_mutex, INFINITE) != WAIT_OBJECT_0)
	{
		printf("[E]\tError claiming write mutex\n");
		return 1;
	}
	return 0;
}

int serial_port_writeUnlock(serial_port_t *sp)
{
	if(!ReleaseMutex(sp->write_mutex))
	{
		printf("[E]\tError releasing write mutex\n");
		return 1;
	}
	return 0;
}

#include "serial_interface.h"
#include <algorithm>

size_t serial_port_read(serial_port_t *sp, std::vector<uint8_t> *buffer)
{
	serial_port_readLock(sp);
	size_t readed = _serial_port_read(sp, buffer);
	serial_port_readUnlock(sp);
	return readed;
}

size_t serial_port_write(serial_port_t *sp, const std::vector<uint8_t> data)
{
	serial_port_readLock(sp);
	serial_port_writeLock(sp);
	size_t writed = _serial_port_write(sp, &data[0], data.size());
	serial_port_writeUnlock(sp);
	serial_port_readUnlock(sp);
	return writed;
}

void serial_port_setPort(serial_port_t *sp, const std::string *port)
{
	serial_port_readLock(sp);
	serial_port_writeLock(sp);
	bool was_open = sp->is_open_;
	if(was_open) serial_port_close(sp);
	_serial_port_setPort(sp, port);
	if(was_open) serial_port_open(sp);
	serial_port_writeUnlock(sp);
	serial_port_readUnlock(sp);
}

void serial_port_flush(serial_port_t *sp)
{
	serial_port_readLock(sp);
	serial_port_writeLock(sp);
	_serial_port_flush(sp);
	serial_port_writeUnlock(sp);
	serial_port_readUnlock(sp);
}

void serial_port_flushInput(serial_port_t *sp)
{
	serial_port_readLock(sp);
	_serial_port_flushInput(sp);
	serial_port_readUnlock(sp);
}

void serial_port_flushOutput(serial_port_t *sp)
{
	serial_port_writeLock(sp);
	_serial_port_flushOutput(sp);
	serial_port_writeUnlock(sp);
}

#endif // #if defined(_WIN32)

