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

#define BUFFERSIZE 		(50)
#define AVAIL_PARAMS 	(10)
#define DELAY_TIME 		(sleep(1))

static struct avail_args_t {
	char s_portname[20];	/* -D, serial port */
	bool bw;				/* -B, bandwith */
	float transmit_freq;	/* -T, transmit frequency */
	float receive_freq;		/* -R, receive frequency */
	char tx_subaud[5];		/* -t, tx subaudio code */
	char rx_subaud[5];		/* -r, rx subaudio code */
	uint8_t squelch;		/* -s, squelch */
	float scan_freq;		/* -S, scan frequency */
	uint8_t volume;			/* -V, volume */
	char filter[6];			/* -F, filter configuration */
	bool tail;				/* -L, tailtone */
} avail_args;

/* Made global for convinient modification */
static const char *opt_string = "D:BLT:R:t:r:s:S:V:F:";
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
	{ "tail", 		no_argument, 		NULL, 'L' },
	{ "rssi", 		no_argument, 		NULL, 0 },
	{ "debug", 		no_argument, 		NULL, 0 },
	{ "version", 	no_argument, 		NULL, 0 },
	{ NULL, 		no_argument, 		NULL, '0' }
};

static int32_t 	debug_flag = 0;
static int32_t 	rssi_flag = 0;
static int32_t 	version_flag = 0;

static int32_t 	set_interface_attribs(int fd, int speed, int parity, int should_block);
static void 	exit_serial_port_not_specified(void);
static int32_t 	check_command_result(char *buf);
static inline void 	clear_buff(char *buf);
static void 	send_command(int32_t fdesc, char *tbuf, char *rbuf);
static int32_t 	check_config(const char *cfg_name, struct avail_args_t *avargs);
static int32_t 	get_parameters_from_args(int ac, char *av[], struct avail_args_t *avargs);
static int32_t 	write_config_file(const char *cfg_name, struct avail_args_t *avargs);

int32_t main(int argc, char *argv[])
{
	/* Constants */
	static const char *DMOCONNECT = "AT+DMOCONNECT";
	static const char *DMOSETGROUP = "AT+DMOSETGROUP";
	static const char *DMOSETVOLUME = "AT+DMOSETVOLUME";
	static const char *SCANFREQ = "S+";
	static const char *SETFILTER = "AT+SETFILTER";
	/*static const char *SETTAIL = "AT+SETTAIL";*/
	static const char *GETRSSI = "RSSI?";
	static const char *GETVERSION = "AT+VERSION";

	static const char *conf_file = "last.cfg";

	int32_t fd;
	char received[BUFFERSIZE] = {0};
	char transmit[BUFFERSIZE] = {0};
	bool success = true;

	/* Check if the last or default config is available */
	if (EXIT_FAILURE == check_config(conf_file, &avail_args))
	{
		return EXIT_FAILURE;
	}

	/* Get parameters from arguments */
	get_parameters_from_args(argc, argv, &avail_args);

	/* Check if 'serial port' option (-D) is entered, open and configure serial port */
	if ((NULL == avail_args.s_portname) && (!debug_flag))
	{
		exit_serial_port_not_specified();
	} else {
		fd = open(avail_args.s_portname, O_RDWR | O_NOCTTY | O_SYNC);
	}
	if ((fd < 0) && (!debug_flag))
	{
		printf("Error when opening %s: %s", avail_args.s_portname, strerror (errno));
		return -errno;
	}

	if (!debug_flag)
	{
		/* set speed to 9600 bps, 8n1 (no parity), blocking */
		set_interface_attribs(fd, B9600, 0, 1);
	}

	printf("Configuring sa818 module, wait...\n");

	sprintf(transmit, "%s\r\n", DMOCONNECT);
	send_command(fd, transmit, received);
	if (EXIT_SUCCESS != check_command_result(received))
	{
		printf("%s failed!\n", DMOCONNECT);
		return(EXIT_FAILURE);
	}

	if (rssi_flag)
	{
		sprintf(transmit, "%s\r\n", GETRSSI);
		send_command(fd, transmit, received);
		printf("%s", received);
	}

	if (version_flag)
	{
		sprintf(transmit, "%s\r\n", GETVERSION);
		send_command(fd, transmit, received);
		printf("%s", received);
	}

	if (rssi_flag || version_flag)
	{
		return EXIT_SUCCESS;
	}

	sprintf(transmit, "%s=%d,%3.4f,%3.4f,%s,%d,%s\r\n", DMOSETGROUP, avail_args.bw, \
		avail_args.transmit_freq, avail_args.receive_freq, avail_args.tx_subaud, avail_args.squelch, avail_args.rx_subaud);
	send_command(fd, transmit, received);
	if (EXIT_SUCCESS != check_command_result(received))
	{
		printf("%s failed!\n", DMOSETGROUP);
		success = false;
	}

	sprintf(transmit, "%s%3.4f\r\n", SCANFREQ, avail_args.scan_freq);
	send_command(fd, transmit, received);
	if (EXIT_SUCCESS != check_command_result(received))
	{
		printf("\nRadiochannel at the %3.4f MHz is NOT FOUND!\n\n", avail_args.scan_freq);
	} else {
		printf("\nRadiochannel at the %3.4f MHz is FOUND!\n\n", avail_args.scan_freq);
	}

	sprintf(transmit, "%s=%d\r\n", DMOSETVOLUME, avail_args.volume);
	send_command(fd, transmit, received);
	if (EXIT_SUCCESS != check_command_result(received))
	{
		printf("%s failed!\n", DMOSETVOLUME);
		success = false;
	}

	sprintf(transmit, "%s=%s\r\n", SETFILTER, avail_args.filter);
	send_command(fd, transmit, received);
	if (EXIT_SUCCESS != check_command_result(received))
	{
		printf("%s failed!\n", SETFILTER);
		success = false;
	}

/*
	sprintf(transmit, "%s=%d\r\n", SETTAIL, avail_args.tail);
	send_command(fd, transmit, received);
	if (EXIT_SUCCESS != check_command_result(received))
	{
		printf("%s failed!\n", SETTAIL);
		success = false;
	}
*/

	if (true == success)
	{
		printf("sa818-V module configuration successfull!\n");
		printf("Writing configuration to the %s\n", conf_file);
		write_config_file(conf_file, &avail_args);
	}

	close(fd);

	return EXIT_SUCCESS;
}

static int32_t write_config_file(const char *cfg_name, struct avail_args_t *avargs)
{
	FILE *last_config = fopen(cfg_name, "w+");
	fprintf(last_config, "\n%d\n%3.4f\n%3.4f\n%s\n%s\n%hhu\n%3.4f\n%hhu\n%s\n%d\n",
			avargs->bw,
			avargs->transmit_freq,
			avargs->receive_freq,
			avargs->tx_subaud,
			avargs->rx_subaud,
			avargs->squelch,
			avargs->scan_freq,
			avargs->volume,
			avargs->filter,
			avargs->tail);
	fclose(last_config);
	return EXIT_SUCCESS;
}

static int32_t get_parameters_from_args(int ac, char *av[], struct avail_args_t *avargs)
{
	int32_t opt = 0;
	int32_t opt_index = 0;

	opt = getopt_long(ac, av, opt_string, sa818_opts, &opt_index);
	if (-1 == opt)
	{
		exit_serial_port_not_specified();
	}
	while(opt != -1)
	{
		switch(opt)
		{
			case 0:
				if( strcmp( "debug", sa818_opts[opt_index].name ) == 0 ) {
					debug_flag = 1;
				}
				if( strcmp( "version", sa818_opts[opt_index].name ) == 0 ) {
					version_flag = 1;
				}
				if( strcmp( "rssi", sa818_opts[opt_index].name ) == 0 ) {
					rssi_flag = 1;
				}
				break;
			case 'D':
				sprintf(avargs->s_portname, "%s", (char *)optarg);
				break;
			case 'B':
				avargs->bw  = !avargs->bw;
				break;
			case 'L':
				avargs->tail  = !avargs->tail;
				break;
			case 'T':
				avargs->transmit_freq = atof(optarg);
				break;
			case 'R':
				avargs->receive_freq = atof(optarg);
				break;
			case 't':
				sprintf(avargs->tx_subaud, "%s", (char *)optarg);
				break;
			case 'r':
				sprintf(avargs->rx_subaud, "%s", (char *)optarg);
				break;
			case 's':
				avargs->squelch = atoi(optarg);
				break;
			case 'S':
				avargs->scan_freq = atof(optarg);
				break;
			case 'V':
				avargs->volume = atoi(optarg);
				break;
			case 'F':
				sprintf(avargs->filter, "%s", (char *)optarg);
				break;
			default:
				break;
		}
		opt = getopt_long(ac, av, opt_string, sa818_opts, &opt_index);
	}

	return opt;
}

static int32_t check_config(const char *cfg_name, struct avail_args_t *avargs)
{
	FILE *last_config = fopen(cfg_name, "r+");
	int32_t read_index = 0;
	int32_t temp_bool_bw = 0;
	int32_t temp_bool_tail = 0;

	read_index = fscanf(last_config, "\n%d\n%f\n%f\n%s\n%s\n%hhu\n%f\n%hhu\n%s\n%d",
			&temp_bool_bw, /* Using 'temp_bool' besause we can't scanf into bool type */
			&avargs->transmit_freq,
			&avargs->receive_freq,
			avargs->tx_subaud,
			avargs->rx_subaud,
			&avargs->squelch,
			&avargs->scan_freq,
			&avargs->volume,
			avargs->filter,
			&temp_bool_tail); /* Using 'temp_bool' besause we can't scanf into bool type */
	if (AVAIL_PARAMS == read_index)
	{
		avargs->bw = temp_bool_bw;
		avargs->tail = temp_bool_tail;
	} else {
		/* Default config for sa818-V */
		printf("Config file %s is incorrect or not found, created default config, restart!\n", cfg_name);
		avargs->s_portname[0] = '\0';
		avargs->bw = false;
		avargs->transmit_freq = 150.0000;
		avargs->receive_freq = 150.0000;
		sprintf(avargs->tx_subaud, "0000");
		sprintf(avargs->rx_subaud, "0000");
		avargs->squelch = 4;
		avargs->scan_freq = 150.0000;
		avargs->volume = 8;
		sprintf(avargs->filter, "0,0,0");
		avargs->tail = false;
		freopen(cfg_name, "w", last_config);
		fprintf(last_config, "\n%d\n%3.4f\n%3.4f\n%s\n%s\n%hhu\n%3.4f\n%hhu\n%s\n%d\n",
			avargs->bw,
			avargs->transmit_freq,
			avargs->receive_freq,
			avargs->tx_subaud,
			avargs->rx_subaud,
			avargs->squelch,
			avargs->scan_freq,
			avargs->volume,
			avargs->filter,
			avargs->tail);
			fclose(last_config);
			return EXIT_FAILURE;
	}
	fclose(last_config);
	return EXIT_SUCCESS;
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

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; /* 8-bit chars
		disable IGNBRK for mismatched speed tests; otherwise receive break as \000 chars */
	tty.c_iflag &= ~IGNBRK;			/* disable break processing */
	tty.c_lflag = 0;				/* no signaling chars, no echo,  no canonical processing */
	tty.c_oflag = 0;				/* no remapping, no delays */
	tty.c_cc[VMIN]  = should_block ? 1 : 0; /* read doesn't block */
	tty.c_cc[VTIME] = 5; 			/* 0.5 seconds read timeout */

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); /* shut off xon/xoff ctrl */

	tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls, enable reading */
	tty.c_cflag &= ~(PARENB | PARODD); /* shut off parity */
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
		printf("error %d from tcsetattr", errno);
		return -errno;
	}
	return EXIT_SUCCESS;
}

static void exit_serial_port_not_specified(void)
{
	puts("Need to specify a serial port with -D /dev/ttyXXX parameter!");
	exit(EINVAL);
}

static int32_t check_command_result(char *buf)
{
	int32_t result = EXIT_SUCCESS;

	if (NULL == strchr(buf, '0'))
	{
		result = !EXIT_SUCCESS;
	}
	clear_buff(buf);

	if (debug_flag)
	{
		result = EXIT_SUCCESS;
	}

	return result;
}

static inline void clear_buff(char *buf)
{
	memset(buf, 0, BUFFERSIZE);
}

static void send_command(int32_t fdesc, char *tbuf, char *rbuf)
{
	if (!debug_flag)
	{
		write(fdesc, tbuf, strlen(tbuf));
		DELAY_TIME;
		read(fdesc, rbuf, BUFFERSIZE);
		DELAY_TIME;
	} else {
		printf("%s\n", tbuf);
	}
}
