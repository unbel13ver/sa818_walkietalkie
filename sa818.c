#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdint.h>

#define BUFFERSIZE 		50

static struct avail_args_t {
	char *s_portname;		/* -D, serial port */
	bool bw;				/* -B, bandwith */
	float transmit_freq;	/* -T, transmit frequency */
	float receive_freq;		/* -R, receive frequency */
	char *tx_subaud;		/* -t, tx subaudio code */
	char *rx_subaud;		/* -r, rx subaudio code */
	uint8_t squelch;			/* -s, squelch */
	float scan_freq;		/* -S, scan frequency */
	uint8_t volume;			/* -V, volume */
	char *filter;			/* -F, filter configuration */
} avail_args;

static const char *opt_string = "D:BT:R:t:r:s:S:V:F:";
static const char *DMOCONNECT = "AT+DMOCONNECT";
static const char *DMOSETGROUP = "AT+DMOSETGROUP";
static const char *DMOSETVOLUME = "AT+DMOSETVOLUME";
static const char *SCANFREQ = "S+";
static const char *SETFILTER = "AT+SETFILTER";


static const struct option sa818_opts[] = {
	{ "portname", 	required_argument, 	NULL, 'D'},
	{ "bandwith", 	no_argument, 		NULL, 'B' },
	{ "tfreq", 		optional_argument, 	NULL, 'T' },
	{ "rfreq", 		optional_argument, 	NULL, 'R' },
	{ "txsub", 		optional_argument, 	NULL, 't' },
	{ "rxsub", 		optional_argument, 	NULL, 'r' },
	{ "squelch", 	optional_argument, 	NULL, 's' },
	{ "scan_freq", 	optional_argument, 	NULL, 'S' },
	{ "volume", 	optional_argument, 	NULL, 'V' },
	{ "filter", 	optional_argument, 	NULL, 'F' },
	{ NULL, 		no_argument, 		NULL, '0' }
};

static int32_t 	set_interface_attribs(int fd, int speed, int parity, int should_block);
static void 		exit_serial_port_not_specified(void);
static void 		check_command_result(char *buf, const char *command);
static void 		clear_buff(char *buf);

static bool successful_excange = true;

int main(int argc, char *argv[])
{
	char received[BUFFERSIZE] = {0};
	char transmit[BUFFERSIZE] = {0};
	int fd;
	int opt = 0;
	int opt_index = 0;

	/* Default config for sa818-V */
	avail_args.s_portname = NULL;
	avail_args.bw = false;
	avail_args.transmit_freq = 150.0000;
	avail_args.receive_freq = 150.0000;
	avail_args.tx_subaud = "0000";
	avail_args.rx_subaud = "0000";
	avail_args.squelch = 4;
	avail_args.scan_freq = 150.0000;
	avail_args.volume = 8;
	avail_args.filter = "0,0,0";

	opt = getopt_long(argc, argv, opt_string, sa818_opts, &opt_index);
	if (-1 == opt)
	{
		exit_serial_port_not_specified();
	}
	while(opt != -1)
	{
		switch(opt)
		{
			case 'D':
				avail_args.s_portname = optarg;
				break;
			case 'B':
				avail_args.bw  = true;
				break;
			case 'T':
				avail_args.transmit_freq = atof(optarg);
				break;
			case 'R':
				avail_args.receive_freq = atof(optarg);
				break;
			case 't':
				avail_args.tx_subaud = optarg;
				break;
			case 'r':
				avail_args.rx_subaud = optarg;
				break;
			case 's':
				avail_args.squelch = atoi(optarg);
				break;
			case 'S':
				avail_args.scan_freq = atof(optarg);
				break;
			case 'V':
				avail_args.volume = atoi(optarg);
				break;
			case 'F':
				avail_args.filter = optarg;
				break;
			default:
				break;
		}
		opt = getopt_long(argc, argv, opt_string, sa818_opts, &opt_index);
	}

	if (NULL == avail_args.s_portname)
	{
		exit_serial_port_not_specified();
	} else {
		fd = open (avail_args.s_portname, O_RDWR | O_NOCTTY | O_SYNC);
	}

	if (fd < 0)
	{
		printf("Error when opening %s: %s", avail_args.s_portname, strerror (errno));
		return -errno;
	}

	set_interface_attribs(fd, B9600, 0, 1);  // set speed to 9600 bps, 8n1 (no parity), blocking

	sprintf(transmit, "%s\r\n", DMOCONNECT);
	printf("%s\n", transmit);
//	write(fd, transmit, strlen(transmit));
//	sleep(1);
//	read(fd, received, sizeof(received));
//	sleep(1);
//	check_command_result(received, DMOCONNECT);

	sprintf(transmit, "%s=%d,%3.4f,%3.4f,%s,%d,%s\r\n", DMOSETGROUP, avail_args.bw, \
		avail_args.transmit_freq, avail_args.receive_freq, avail_args.tx_subaud, avail_args.squelch, avail_args.rx_subaud);
	printf("%s\n", transmit);
//	write(fd, "AT+DMOSETGROUP=0,150.0000,150.0000,0000,0,0000\r\n", 50);
//	write(fd, transmit, strlen(transmit));
//	sleep(1);
//	read(fd, received, sizeof(received));
//	sleep(1);
//	check_command_result(received, DMOSETGROUP);

	sprintf(transmit, "%s%3.4f\r\n", SCANFREQ, avail_args.scan_freq);
	printf("%s\n", transmit);
//	write(fd, "S+150.0000\r\n", 12);
//	write(fd, transmit, strlen(transmit));
//	sleep(1);
//	read(fd, received, sizeof(received));
//	sleep(1);
//	check_command_result(received, SCANFREQ);

	sprintf(transmit, "%s=%d\r\n", DMOSETVOLUME, avail_args.volume);
	printf("%s\n", transmit);
//	write(fd, "AT+DMOSETVOLUME=8\r\n", 19);
//	write(fd, transmit, strlen(transmit));
//	sleep(1);
//	read(fd, received, sizeof(received));
//	sleep(1);
//	check_command_result(received, DMOSETVOLUME);

	sprintf(transmit, "%s=%s\r\n", SETFILTER, avail_args.filter);
	printf("%s\n", transmit);
//	write(fd, "AT+SETFILTER=0,0,0\r\n", 23);
//	write(fd, transmit, strlen(transmit));
//	sleep(1);
//	read(fd, received, sizeof(received));
//	sleep(1);
//	check_command_result(received, SETFILTER);

	close(fd);

	if (true == successful_excange)
	{
		puts("sa818 module configurated sucessful!");
	}

	return 0;
}

static int32_t set_interface_attribs (int fd, int speed, int parity, int should_block)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		printf("error %d from tcgetattr", errno);
		return -errno;
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
		return -errno;
	}
	return 0;
}

static void exit_serial_port_not_specified(void)
{
	puts("Need to specify a serial port with -D /dev/ttyXXX parameter!");
	exit(EINVAL);
}

static void check_command_result(char *buf, const char *command)
{
	if (NULL == strchr(buf, '0'))
	{
		printf("The %s command failed\n", command);
		successful_excange = false;
	}
	clear_buff(buf);
}

static void clear_buff(char *buf)
{
	memset(buf, 0, BUFFERSIZE);
}
