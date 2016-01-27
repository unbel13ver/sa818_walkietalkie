#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int set_interface_attribs (int fd, int speed, int parity, int should_block)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		printf("error %d from tcgetattr", errno);
		return -1;
	}

	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break as \000 chars
	tty.c_iflag &= ~IGNBRK;			// disable break processing
	tty.c_lflag = 0;				// no signaling chars, no echo,  no canonical processing
	tty.c_oflag = 0;				// no remapping, no delays
	tty.c_cc[VMIN]  = should_block ? 1 : 0; // read doesn't block
	tty.c_cc[VTIME] = 5; 			// 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD); // ignore modem controls, enable reading
	tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
		printf("error %d from tcsetattr", errno);
		return -1;
	}
	return 0;
}

int main(void)
{
	char *portname = "/dev/ttyUSB2";
	char received[100] = {0};
	int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);

	if (fd < 0)
	{
		printf("error %d opening %s: %s", errno, portname, strerror (errno));
		return -1;
	}

	set_interface_attribs (fd, B9600, 0, 1);  // set speed to 9600 bps, 8n1 (no parity), blocking

	write(fd, "AT+DMOCONNECT\r\n", 15);
	sleep(1);            
	read(fd, received, sizeof(received));
	printf("\nResponce: %s\n", received);
	memset(received, 0, sizeof(received));

	sleep(1);
	write(fd, "AT+DMOSETGROUP=0,150.0000,150.0000,0000,8,0000\r\n", 50);
	sleep(1);
	read(fd, received, sizeof(received));
	printf("Responce: %s\n", received);
	memset(received, 0, sizeof(received));

	sleep(1);
	write(fd, "S+150.0000\r\n", 12);
	sleep(1);
	read(fd, received, sizeof(received));
	printf("Responce: %s\n", received);
	memset(received, 0, sizeof(received));

	sleep(1);
	write(fd, "AT+DMOSETVOLUME=8\r\n", 19);
	sleep(1);
	read(fd, received, sizeof(received));
	printf("Responce: %s\n", received);
	memset(received, 0, sizeof(received));

	sleep(1);
	write(fd, "AT+SETFILTER=0,0,0\r\n", 23);
	sleep(1);
	read(fd, received, sizeof(received));
	printf("Responce: %s\n", received);
	memset(received, 0, sizeof(received));

	close(fd);

	return 0;
}
