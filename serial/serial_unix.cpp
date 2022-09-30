#include "serial_interface.h"
#include <chrono>
#include <thread>

#ifdef __cplusplus
#define CAST(t, v) (static_cast<t>(v))
#else
#define CAST(t, v) ((t)(v))
#endif

#if !defined(_WIN32)

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <pthread.h>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

#if defined(__linux__)
#include <linux/serial.h>
#endif

#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#ifdef __MACH__
#include <AvailabilityMacros.h>
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#ifndef TIOCINQ
#ifdef FIONREAD
#define TIOCINQ FIONREAD
#else
#define TIOCINQ 0x541B
#endif
#endif

#if defined(MAC_OS_X_VERSION_10_3) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3)
#include <IOKit/serial/ioss.h>
#endif

MillisecondTimer::MillisecondTimer(const uint32_t millis)
	: expiry(timespec_now())
{
	int64_t tv_nsec = expiry.tv_nsec + (millis * 1e6);
	if(tv_nsec >= 1e9)
	{
		int64_t sec_diff = tv_nsec / static_cast<int>(1e9);
		expiry.tv_nsec = tv_nsec % static_cast<int>(1e9);
		expiry.tv_sec += sec_diff;
	}
	else
	{
		expiry.tv_nsec = tv_nsec;
	}
}

int64_t MillisecondTimer::remaining()
{
	timespec now(timespec_now());
	int64_t millis = (expiry.tv_sec - now.tv_sec) * 1e3;
	millis += (expiry.tv_nsec - now.tv_nsec) / 1e6;
	return millis;
}

timespec MillisecondTimer::timespec_now()
{
	timespec time;
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	time.tv_sec = mts.tv_sec;
	time.tv_nsec = mts.tv_nsec;
#else
	clock_gettime(CLOCK_MONOTONIC, &time);
#endif
	return time;
}

timespec timespec_from_ms(const uint32_t millis)
{
	timespec time;
	time.tv_sec = millis / 1e3;
	time.tv_nsec = (millis - (time.tv_sec * 1e3)) * 1e6;
	return time;
}

void serial_port_init(serial_port_t *sp,
					  const std::string &port,
					  unsigned long baudrate,
					  bytesize_t bytesize,
					  parity_t parity,
					  stopbits_t stopbits,
					  flowcontrol_t flowcontrol)
{
	sp->port_ = port;
	sp->fd_ = -1;
	sp->is_open_ = false;
	sp->xonxoff_ = false;
	sp->rtscts_ = false;
	sp->baudrate_ = baudrate;
	sp->parity_ = parity;
	sp->bytesize_ = bytesize;
	sp->stopbits_ = stopbits;
	sp->flowcontrol_ = flowcontrol;
	pthread_mutex_init(&sp->read_mutex, NULL);
	pthread_mutex_init(&sp->write_mutex, NULL);
}

int serial_port_deinit(serial_port_t *sp)
{
	int sts = serial_port_close(sp);
	pthread_mutex_destroy(&sp->read_mutex);
	pthread_mutex_destroy(&sp->write_mutex);
	return sts;
}

int serial_port_open(serial_port_t *sp)
{
	if(sp->port_.empty())
	{
		printf("Empty port is invalid\n");
		return -1;
	}
	if(sp->is_open_ == true)
	{
		printf("Serial port already open\n");
		return -2;
	}

	sp->fd_ = open(sp->port_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

	if(sp->fd_ == -1)
	{
		switch(errno)
		{
		case EINTR: return serial_port_open(sp); // Recurse because this is a recoverable error

		case ENFILE:
		case EMFILE:
			printf("[E]\tToo many file handles open\n");
			return -4;

		default:
			printf("[E]\tUnknown error: %d\n", errno);
			return -5;
		}
	}

	serial_port_reconfigure(sp);
	sp->is_open_ = true;
	return 0;
}

int serial_port_reconfigure(serial_port_t *sp)
{
	if(sp->fd_ == -1) return -1;

	struct termios options; // The options for the file descriptor
	if(tcgetattr(sp->fd_, &options) == -1)
	{
		printf("[E]\tError getting the serial port state\n");
		return -2;
	}

	// set up raw mode / no echo / binary
	options.c_cflag |= CAST(tcflag_t, CLOCAL | CREAD);
	options.c_lflag &= CAST(tcflag_t, ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG | IEXTEN)); //|ECHOPRT

	options.c_oflag &= CAST(tcflag_t, ~(OPOST));
	options.c_iflag &= CAST(tcflag_t, ~(INLCR | IGNCR | ICRNL | IGNBRK));
#ifdef IUCLC
	options.c_iflag &= CAST(tcflag_t, ~IUCLC);
#endif
#ifdef PARMRK
	options.c_iflag &= CAST(tcflag_t, ~PARMRK);
#endif

	// setup baud rate
	bool custom_baud = false;
	speed_t baud;
	switch(sp->baudrate_)
	{
	case 0: baud = B0; break;
	case 50: baud = B50; break;
	case 75: baud = B75; break;
	case 110: baud = B110; break;
	case 134: baud = B134; break;
	case 150: baud = B150; break;
	case 200: baud = B200; break;
	case 300: baud = B300; break;
	case 600: baud = B600; break;
	case 1200: baud = B1200; break;
	case 1800: baud = B1800; break;
	case 2400: baud = B2400; break;
	case 4800: baud = B4800; break;
	case 9600: baud = B9600; break;
	case 19200: baud = B19200; break;
	case 57600: baud = B57600; break;
	case 38400: baud = B38400; break;
	case 115200: baud = B115200; break;
	case 128000: baud = B128000; break;
	case 230400: baud = B230400; break;
	case 256000: baud = B256000; break;
	case 460800: baud = B460800; break;
	case 500000: baud = B500000; break;
	case 576000: baud = B576000; break;
	case 921600: baud = B921600; break;
	case 1000000: baud = B1000000; break;
	case 1152000: baud = B1152000; break;
	case 1500000: baud = B1500000; break;
	case 2000000: baud = B2000000; break;
	case 2500000: baud = B2500000; break;
	case 3000000: baud = B3000000; break;
	default:
		custom_baud = true;
#if defined(__linux__) && defined(TIOCSSERIAL)
		struct serial_struct ser;

		if(-1 == ioctl(sp->fd_, TIOCGSERIAL, &ser))
		{
			THROW(IOException, errno);
		}

		// set custom divisor
		ser.custom_divisor = ser.baud_base / CAST(int, sp->baudrate_);
		// update flags
		ser.flags &= ~ASYNC_SPD_MASK;
		ser.flags |= ASYNC_SPD_CUST;

		if(-1 == ioctl(sp->fd_, TIOCSSERIAL, &ser))
		{
			THROW(IOException, errno);
		}
#endif
	}
	if(custom_baud == false)
	{
#ifdef _BSD_SOURCE
		cfsetspeed(&options, baud);
#else
		cfsetispeed(&options, baud);
		cfsetospeed(&options, baud);
#endif
	}

	// setup char len
	options.c_cflag &= (tcflag_t)~CSIZE;
	switch(sp->bytesize_)
	{
	case fivebits: options.c_cflag |= CS5; break;
	case sixbits: options.c_cflag |= CS6; break;
	case sevenbits: options.c_cflag |= CS7; break;
	default:
	case eightbits: options.c_cflag |= CS8; break;
	}

	// setup stopbits
	if(sp->stopbits_ == stopbits_one)
		options.c_cflag &= (tcflag_t) ~(CSTOPB);
	else if(sp->stopbits_ == stopbits_one_point_five)
		// ONE POINT FIVE same as TWO.. there is no POSIX support for 1.5
		options.c_cflag |= (CSTOPB);
	else if(sp->stopbits_ == stopbits_two)
		options.c_cflag |= (CSTOPB);

	// setup parity
	options.c_iflag &= (tcflag_t) ~(INPCK | ISTRIP);
	if(sp->parity_ == parity_none)
	{
		options.c_cflag &= (tcflag_t) ~(PARENB | PARODD);
	}
	else if(sp->parity_ == parity_even)
	{
		options.c_cflag &= (tcflag_t) ~(PARODD);
		options.c_cflag |= (PARENB);
	}
	else if(sp->parity_ == parity_odd)
	{
		options.c_cflag |= (PARENB | PARODD);
	}
#ifdef CMSPAR
	else if(sp->parity_ == parity_mark)
	{
		options.c_cflag |= (PARENB | CMSPAR | PARODD);
	}
	else if(sp->parity_ == parity_space)
	{
		options.c_cflag |= (PARENB | CMSPAR);
		options.c_cflag &= (tcflag_t) ~(PARODD);
	}
#else
	// CMSPAR is not defined on OSX. So do not support mark or space parity.
	else if(sp->parity_ == parity_mark || parity_ == parity_space)
	{
		throw invalid_argument("OS does not support mark or space parity");
	}
#endif // ifdef CMSPAR

	// setup flow control
	if(sp->flowcontrol_ == flowcontrol_none)
	{
		sp->xonxoff_ = false;
		sp->rtscts_ = false;
	}
	if(sp->flowcontrol_ == flowcontrol_software)
	{
		sp->xonxoff_ = true;
		sp->rtscts_ = false;
	}
	if(sp->flowcontrol_ == flowcontrol_hardware)
	{
		sp->xonxoff_ = false;
		sp->rtscts_ = true;
	}
	// xonxoff
#ifdef IXANY
	if(sp->xonxoff_)
		options.c_iflag |= (IXON | IXOFF); //|IXANY)
	else
		options.c_iflag &= CAST(tcflag_t, ~(IXON | IXOFF | IXANY));
#else
	if(xonxoff_)
		options.c_iflag |= (IXON | IXOFF);
	else
		options.c_iflag &= (tcflag_t) ~(IXON | IXOFF);
#endif
		// rtscts
#ifdef CRTSCTS
	if(sp->rtscts_)
		options.c_cflag |= (CRTSCTS);
	else
		options.c_cflag &= CAST(unsigned long, ~(CRTSCTS));
#elif defined CNEW_RTSCTS
	if(rtscts_)
		options.c_cflag |= (CNEW_RTSCTS);
	else
		options.c_cflag &= (unsigned long)~(CNEW_RTSCTS);
#else
#error "OS Support seems wrong."
#endif

	// http://www.unixwiz.net/techtips/termios-vmin-vtime.html
	// this basically sets the read call up to be a polling read,
	// but we are using select to ensure there is data available
	// to read before each call, so we should never needlessly poll
	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 0;

	// activate settings
	::tcsetattr(sp->fd_, TCSANOW, &options);

	// Update byte_time_ based on the new settings.
	uint32_t bit_time_ns = 1e9 / sp->baudrate_;
	sp->byte_time_ns_ = bit_time_ns * (1 + sp->bytesize_ + sp->parity_ + sp->stopbits_);

	// Compensate for the stopbits_one_point_five enum being equal to int 3,
	// and not 1.5.
	if(sp->stopbits_ == stopbits_one_point_five)
	{
		sp->byte_time_ns_ += ((1.5 - stopbits_one_point_five) * bit_time_ns);
	}
	return 0;
}

int serial_port_close(serial_port_t *sp)
{
	if(!sp->is_open_) return -1;
	if(sp->fd_ != -1)
	{
		int ret;
		ret = close(sp->fd_);
		if(ret == 0)
		{
			sp->fd_ = -1;
		}
		else
		{
			printf("[E]\tClose error (%d)\n", ret);
			return -2;
		}
	}
	sp->is_open_ = false;
	return 0;
}

int serial_port_available(serial_port_t *sp)
{
	if(!sp->is_open_) return -1;
	int count = 0;
	if(-1 == ioctl(sp->fd_, TIOCINQ, &count))
	{
		return -2;
	}
	else
	{
		return count;
	}
}

int serial_port_waitReadable(serial_port_t *sp, uint32_t timeout)
{
	// Setup a select call to block for serial data or a timeout
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sp->fd_, &readfds);
	timespec timeout_ts(timespec_from_ms(timeout));
	int r = pselect(sp->fd_ + 1, &readfds, nullptr, nullptr, &timeout_ts, nullptr);
	if(r < 0)
	{
		// Select was interrupted
		if(errno == EINTR) return 0;
		// Otherwise there was some error
		printf("[E]\twaitReadable error %d\n", errno);
		return -1;
	}
	// Timeout occurred
	if(r == 0)
	{
		return 0;
	}
	// This shouldn't happen, if r > 0 our fd has to be in the list!
	if(!FD_ISSET(sp->fd_, &readfds))
	{
		printf("[E]\tselect reports ready to read, but our fd isn't in the list, this shouldn't happen!\n");
		return -2;
	}
	// Data available to read.
	return 1;
}

void serial_port_waitByteTimes(serial_port_t *sp, size_t count)
{
	timespec wait_time = {0, CAST(long, sp->byte_time_ns_ *count)};
	pselect(0, nullptr, nullptr, nullptr, &wait_time, nullptr);
}

int _read(serial_port_t *sp, uint8_t *buf, size_t size)
{
	// If the port is not open, throw
	if(!sp->is_open_) return 0;
	size_t bytes_read = 0;

	// Calculate total timeout in milliseconds t_c + (t_m * N)
	long total_timeout_ms = sp->timeout_.read_timeout_constant;
	total_timeout_ms += sp->timeout_.read_timeout_multiplier * static_cast<long>(size);
	MillisecondTimer total_timeout(total_timeout_ms);

	// Pre-fill buffer with available bytes
	{
		ssize_t bytes_read_now = read(sp->fd_, buf, size);
		if(bytes_read_now > 0)
		{
			bytes_read = bytes_read_now;
		}
	}

	while(bytes_read < size)
	{
		int64_t timeout_remaining_ms = total_timeout.remaining();
		if(timeout_remaining_ms <= 0)
		{
			// Timed out
			break;
		}
		// Timeout for the next select is whichever is less of the remaining
		// total read timeout and the inter-byte timeout.
		uint32_t timeout = std::min(static_cast<uint32_t>(timeout_remaining_ms),
									sp->timeout_.inter_byte_timeout);
		// Wait for the device to be readable, and then attempt to read.
		int r = serial_port_waitReadable(sp, timeout);
		if(r < 0)
		{
			return r;
		}
		if(r)
		{
			// If it's a fixed-length multi-byte read, insert a wait here so that
			// we can attempt to grab the whole thing in a single IO call. Skip
			// this wait if a non-max inter_byte_timeout is specified.
			if(size > 1 && sp->timeout_.inter_byte_timeout == Timeout::max())
			{
				int bytes_available = serial_port_available(sp);
				if(bytes_available < 0) return bytes_available;
				if(bytes_available + bytes_read < size)
				{
					serial_port_waitByteTimes(sp, size - (bytes_available + bytes_read));
				}
			}
			// This should be non-blocking returning only what is available now
			//  Then returning so that select can block again.
			ssize_t bytes_read_now = read(sp->fd_, buf + bytes_read, size - bytes_read);
			// read should always return some data as select reported it was
			// ready to read when we get to this point.
			if(bytes_read_now < 1)
			{
				// Disconnected devices, at least on Linux, show the
				// behavior that they are always ready to read immediately
				// but reading returns nothing.
				printf("[E]\tDevice reports readiness to read but returned no data (device disconnected?)\n");
				return -3;
			}
			// Update bytes_read
			bytes_read += static_cast<size_t>(bytes_read_now);
			// If bytes_read == size then we have read everything we need
			if(bytes_read == size)
			{
				break;
			}
			// If bytes_read < size then we have more to read
			if(bytes_read < size)
			{
				continue;
			}
			// If bytes_read > size then we have over read, which shouldn't happen
			if(bytes_read > size)
			{
				printf("[E]\tRead over read, too many bytes where read, this shouldn't happen, might be a logical error!\n");
				return -4;
			}
		}
	}
	return bytes_read;
}

size_t _serial_port_write(serial_port_t *sp, const uint8_t *data, size_t length)
{
	if(sp->is_open_ == false) return 0;
	fd_set writefds;
	size_t bytes_written = 0;

	// Calculate total timeout in milliseconds t_c + (t_m * N)
	long total_timeout_ms = sp->timeout_.write_timeout_constant;
	total_timeout_ms += sp->timeout_.write_timeout_multiplier * static_cast<long>(length);
	MillisecondTimer total_timeout(total_timeout_ms);

	bool first_iteration = true;
	while(bytes_written < length)
	{
		int64_t timeout_remaining_ms = total_timeout.remaining();
		// Only consider the timeout if it's not the first iteration of the loop
		// otherwise a timeout of 0 won't be allowed through
		if(!first_iteration && (timeout_remaining_ms <= 0))
		{
			// Timed out
			break;
		}
		first_iteration = false;

		timespec timeout(timespec_from_ms(timeout_remaining_ms));

		FD_ZERO(&writefds);
		FD_SET(sp->fd_, &writefds);

		// Do the select
		int r = pselect(sp->fd_ + 1, nullptr, &writefds, nullptr, &timeout, nullptr);

		// Figure out what happened by looking at select's response 'r'
		/** Error **/
		if(r < 0)
		{
			// Select was interrupted, try again
			if(errno == EINTR)
			{
				continue;
			}
			// Otherwise there was some error
			printf("[E]\tError! %d\n", errno);
			return 0;
		}
		/** Timeout **/
		if(r == 0)
		{
			break;
		}
		/** Port ready to write **/
		if(r > 0)
		{
			// Make sure our file descriptor is in the ready to write list
			if(FD_ISSET(sp->fd_, &writefds))
			{
				// This will write some
				ssize_t bytes_written_now =
					::write(sp->fd_, data + bytes_written, length - bytes_written);
				// write should always return some data as select reported it was
				// ready to write when we get to this point.
				if(bytes_written_now < 1)
				{
					// Disconnected devices, at least on Linux, show the
					// behavior that they are always ready to write immediately
					// but writing returns nothing.
					printf("[E]\tDevice reports readiness to write but returned no data (device disconnected)\n");
					return 0;
				}
				// Update bytes_written
				bytes_written += static_cast<size_t>(bytes_written_now);
				// If bytes_written == size then we have written everything we need to
				if(bytes_written == length)
				{
					break;
				}
				// If bytes_written < size then we have more to write
				if(bytes_written < length)
				{
					continue;
				}
				// If bytes_written > size then we have over written, which shouldn't happen
				if(bytes_written > length)
				{
					printf("[E]\tWrite over wrote, too many bytes where written, this shouldn't happen, might be a logical error!\n");
					return 0;
				}
			}
			// This shouldn't happen, if r > 0 our fd has to be in the list!
			printf("[E]\tselect reports ready to write, but our fd isn't in the list, this shouldn't happen!\n");
			return 0;
		}
	}
	return bytes_written;
}

void _serial_port_setPort(serial_port_t *sp, const std::string &port) { sp->port_ = port; }

std::string _serial_port_getPort(serial_port_t *sp) { return sp->port_; }

void _serial_port_flush(serial_port_t *sp)
{
	if(sp->is_open_ == false) return;
	tcdrain(sp->fd_);
}

void _serial_port_flushInput(serial_port_t *sp)
{
	if(sp->is_open_ == false) return;
	tcflush(sp->fd_, TCIFLUSH);
}

void _serial_port_flushOutput(serial_port_t *sp)
{
	if(sp->is_open_ == false) return;
	tcflush(sp->fd_, TCOFLUSH);
}

void serial_port_sendBreak(serial_port_t *sp, int duration)
{
	if(sp->is_open_ == false) return;
	tcsendbreak(sp->fd_, static_cast<int>(duration / 4));
}

int serial_port_setBreak(serial_port_t *sp, bool level)
{
	if(sp->is_open_ == false) return -1;
	if(level)
	{
		if(-1 == ioctl(sp->fd_, TIOCSBRK))
		{
			printf("[E]\tsetBreak failed on a call to ioctl(TIOCSBRK): %d\n", errno);
			return -2;
		}
	}
	else
	{
		if(-1 == ioctl(sp->fd_, TIOCCBRK))
		{
			printf("[E]\tsetBreak failed on a call to ioctl(TIOCCBRK): %d\n", errno);
			return -3;
		}
	}
	return 0;
}

int serial_port_setRTS(serial_port_t *sp, bool level)
{
	if(sp->is_open_ == false) return -1;
	int command = TIOCM_RTS;
	if(level)
	{
		if(-1 == ioctl(sp->fd_, TIOCMBIS, &command))
		{
			printf("[E]\tsetRTS failed on a call to ioctl(TIOCMBIS): %d\n", errno);
			return -2;
		}
	}
	else
	{
		if(-1 == ioctl(sp->fd_, TIOCMBIC, &command))
		{
			printf("[E]\tsetRTS failed on a call to ioctl(TIOCMBIC): %d\n", errno);
			return -3;
		}
	}
	return 0;
}

int serial_port_setDTR(serial_port_t *sp, bool level)
{
	if(sp->is_open_ == false) return -1;
	int command = TIOCM_DTR;
	if(level)
	{
		if(-1 == ioctl(sp->fd_, TIOCMBIS, &command))
		{
			printf("[E]\tsetDTR failed on a call to ioctl(TIOCMBIS): %d\n", errno);
			return -2;
		}
	}
	else
	{
		if(-1 == ioctl(sp->fd_, TIOCMBIC, &command))
		{
			printf("[E]\tsetDTR failed on a call to ioctl(TIOCMBIC): %d\n", errno);
			return -3;
		}
	}
	return 0;
}

bool serial_port_waitForChange(serial_port_t *sp)
{
#ifndef TIOCMIWAIT
	while(sp->is_open_ == true)
	{
		int status;
		if(-1 == ioctl(sp->fd_, TIOCMGET, &status))
		{
			printf("[E]\twaitForChange failed on a call to ioctl(TIOCMGET): %d %s\n", errno, strerror(errno));
			return false;
		}
		if(0 != (status & TIOCM_CTS) || 0 != (status & TIOCM_DSR) || 0 != (status & TIOCM_RI) || 0 != (status & TIOCM_CD)) return true;

		std::this_thread::sleep_for(std::chrono::microseconds(1000));
	}
	return false;
#else
	int command = (TIOCM_CD | TIOCM_DSR | TIOCM_RI | TIOCM_CTS);
	if(-1 == ioctl(sp->fd_, TIOCMIWAIT, &command))
	{
		stringstream ss;
		ss << "waitForDSR failed on a call to ioctl(TIOCMIWAIT): "
		   << errno << " " << strerror(errno);
		throw(SerialException(ss.str().c_str()));
	}
	return true;
#endif
}

bool serial_port_getCTS(serial_port_t *sp)
{
	if(sp->is_open_ == false) return false;
	int status;
	if(-1 == ioctl(sp->fd_, TIOCMGET, &status))
	{
		printf("[E]\tError getting the status of the CTS line\n");
	}
	return 0 != (status & TIOCM_CTS);
}

bool serial_port_getDSR(serial_port_t *sp)
{
	if(sp->is_open_ == false) return false;
	int status;
	if(-1 == ioctl(sp->fd_, TIOCMGET, &status))
	{
		printf("[E]\tError getting the status of the DSR line\n");
	}
	return 0 != (status & TIOCM_DSR);
}

bool serial_port_getRI(serial_port_t *sp)
{
	if(sp->is_open_ == false) return false;
	int status;
	if(-1 == ioctl(sp->fd_, TIOCMGET, &status))
	{
		printf("[E]\tError getting the status of the RI line\n");
	}
	return 0 != (status & TIOCM_RI);
}

bool serial_port_getCD(serial_port_t *sp)
{
	if(sp->is_open_ == false) return false;
	int status;
	if(-1 == ioctl(sp->fd_, TIOCMGET, &status))
	{
		printf("[E]\tError getting the status of the CD line\n");
	}
	return 0 != (status & TIOCM_CD);
}

int serial_port_readLock(serial_port_t *sp)
{
	int result = pthread_mutex_lock(&sp->read_mutex);
	if(result)
	{
		printf("==========");
		return result;
	}
	return 0;
}

int serial_port_readUnlock(serial_port_t *sp)
{
	int result = pthread_mutex_unlock(&sp->read_mutex);
	if(result)
	{
		printf("==========");
		return result;
	}
	return 0;
}

int serial_port_writeLock(serial_port_t *sp)
{
	int result = pthread_mutex_lock(&sp->write_mutex);
	if(result)
	{
		printf("==========");
		return result;
	}
	return 0;
}

int serial_port_writeUnlock(serial_port_t *sp)
{
	int result = pthread_mutex_unlock(&sp->write_mutex);
	if(result)
	{
		printf("==========");
		return result;
	}
	return 0;
}

size_t _serial_port_read(serial_port_t *sp, std::vector<uint8_t> *buf)
{
	size_t count = CAST(size_t, _read(sp, sp->buf_rx, sizeof(sp->buf_rx)));
	buf->assign(sp->buf_rx, sp->buf_rx + count);
	return count;
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
	_serial_port_setPort(sp, *port);
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

#endif // !defined(_WIN32)
