/*
 * Hayden Kroepfl - 2017
 */
#include "serial.h"
#include "log.h"

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

static int serfd;
static const char *device = "/dev/ttyS0";
static const speed_t baud = B19200;

/*
 * serial_init - Open serial port for read/write
 */
int serial_init(void) 
{
	struct termios ser;
	
	serfd = open(device, O_RDWR);
	if (serfd < 0) {
		logerror("Unable to open device %s: [%d] %s", device, errno, strerror(errno));
		return -1;
	}
	if (tcgetattr(serfd, &ser) < 0) {
		logerror("Unable to get termios attributes: [%d] %s", errno, strerror(errno));
		return -1;
	}
	
	cfmakeraw(&ser);	     // Set raw mode instead of canonical
	cfsetspeed(&ser, baud);
	// Set up 8N1 serial
	ser.c_cflag &= ~(CSIZE | CSTOPB | PARENB); // Clear size, 1 stop bit, no parity
	ser.c_cflag |= CS8 | CLOCAL; // 8 bit, ignore modem lines
	
	if (tcsetattr(serfd, TCSANOW, &ser) < 0) {
		logerror("Unable to set termios attributes: [%d] %s", errno, strerror(errno));
		return -1;
	}
	return 0;
}

/*
 * serial_close - Close serial port
 */
void serial_close(void) 
{
	if (serfd >= 0) {
		close(serfd);
	}
}

/*
 * serial_read - Read into a buffer from the serial port
 */
int serial_read(uint8_t *buf, int len)
{
	int count = 0;
	int bytes;
	
	while ((bytes = read(serfd, buf+count, len-count)) > 0 && count < len) {
		count += bytes;
	}
	if (bytes < 0) {
		logerror("Unable to read from serial port: [%d] %s", errno, strerror(errno));
		return -1;
	}
	return count;
}

/*
 * serial_read_byte - Read a byte from the serial port
 */
uint8_t serial_read_byte(void)
{
	uint8_t val;
	
	if (read(serfd, &val, 1) < 1) {
		logerror("Unable to read byte from serial port: [%d] %s", errno, strerror(errno));
		return 0;
	}
	
	return val;
}

/*
 * serial_write_byte - Write a byte to the serial port
 */
void serial_write_byte(uint8_t byte)
{
	if (write(serfd, &byte, 1) < 1) {
		logerror("Unable to write byte to serial port: [%d] %s", errno, strerror(errno));
	}
}
/*
 * serial_write - Write buffer to serial port
 */
int serial_write(uint8_t *buf, int len)
{
	int count = 0;
	int bytes;
	
	while ((bytes = write(serfd, buf+count, len-count)) > 0 && count < len) {
		count += bytes;
	}
	if (bytes < 0) {
		logerror("Unable to write to serial port: [%d] %s", errno, strerror(errno));
		return -1;
	}
	return count;
}
