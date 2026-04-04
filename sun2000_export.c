/*
 *
 * Huawei SUN2000 12ktl-m0 MODBUS TCP
 *
 * Read and set export limitation
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <modbus/modbus.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/* Validate input export limitation */
#define LOW_LIMIT 0
#define HIGH_LIMIT 12

modbus_t *ctx;
int debug;

/* Check that the SDongle05 IP/TCP port is reachable */
int check_port(int portno, char *hostname)
{
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
	}

	server = gethostbyname(hostname);

	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
			(char *)&serv_addr.sin_addr.s_addr,
			server->h_length);

	serv_addr.sin_port = htons(portno);
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		close(sockfd);
		return 0;
	} else {
		close(sockfd);
		return 1;
	}

}

/* Read Modbus 2 byte register */
int read_active_power(float *apwr)
{
	uint16_t reg[2];

	/* 
	   Active Power		32080
	   Power limit		47416
	   */

	if(modbus_read_registers(ctx, 47416, 2, reg) == -1)
	{
		fprintf(stderr, "Modbus read failed\n");
		return 0;
	}

	int32_t raw = ((int32_t)reg[0] << 16) | reg[1];
	*apwr=(float )(raw / 1000.0);

	if(debug)
	{
		printf("raw %d\n", raw);
		printf("low %d\n", reg[1]);
		printf("high %d\n", reg[0]);
		printf("apwr %f\n", *apwr);
	}

	return 1;
}

/* Read Modbus 1 byte register (not used) */
float read_reg()
{
	uint16_t reg[2];

	if(modbus_read_registers(ctx, 47415, 1, reg) == -1)
	{
		printf("Modbus read failed\n");
		return 0;
	}

	return reg[0];
}

/* Write export limit value to 2 byte Modbus register */
int set_power_limit(int kw)
{
	uint16_t reg[2];
	reg[0]=0;
	reg[1]=kw;

	if (debug)
	{
		printf("low %d\n", reg[1]);
		printf("high %d\n", reg[0]);
	}

	if(modbus_write_registers(ctx, 47416, 2, reg) == -1)
	{
		fprintf(stderr, "Modbus write failed\n");
		return 0;
	}
	if (debug)
		printf("Power limit set to #%d#\n", kw);
	return 1;
}

/* Close and free Modbus, then exit */
void modbus_terminate_and_exit(int exitcode)
{
	modbus_close(ctx);
	modbus_free(ctx);
	exit(exitcode);
}

/* Signal actions so the Modbus connection will be terminated before program exit */
void sigint_handler(int signal)
{
	int rc;

	if (signal == SIGINT)
		printf("\nIntercepted SIGINT!\n");
	if (signal == SIGHUP)
		printf("\nIntercepted SIGHUP!\n");
	if (signal == SIGTERM)
		printf("\nIntercepted SIGTERM!\n");
	modbus_terminate_and_exit(1);
}

/* Catch signals to invoke sigint_handler */
void set_signal_action(void)
{
	// Declare the sigaction structure
	struct sigaction act;

	// Set all of the structure's bits to 0 to avoid errors
	// relating to uninitialized variables...
	bzero(&act, sizeof(act));
	// Set the signal handler as the default action
	act.sa_handler = &sigint_handler;
	// Apply the action in the structure to the
	// SIGINT signal (ctrl-c)
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
}


int main(int argc, char *argv[] )
{
	float apwr;
	int n, rc, opt;
	int set_export;
	char *export_value;
	int modbus_slave=1;
	int modbus_port=502;
	char dongle_ip[512];
	char* end;

	/* Argument mapping */
	while ((opt = getopt(argc, argv, "ds:i:m:p:h")) != -1) {
		switch(opt) {
			case 'd' : debug = 1; break;
			case 'i' : strncpy(dongle_ip, optarg, sizeof(dongle_ip)) ; break;
			case 'm' : modbus_slave = (int )strtol (optarg, &end, 10);
				   if (optarg==end)
				   {
					   fprintf(stderr,"%s: -m illegal value %s\n", argv[0], optarg);
					   exit(-1);
				   }
				   break;
			case 'p' : modbus_port = (int )strtol (optarg, &end, 10);
				   if (optarg==end)
				   {
					   fprintf(stderr,"%s: -p illegal value %s\n", argv[0], optarg);
					   exit(-1);
				   }
				   break;
			case 's' : export_value = optarg;
				   set_export=1; break;
			case 'h' : printf("Usage: \n%s\n%s\n%s\n%s\n%s\n%s\n",\
						   "-d debug",
						   "-i dongle_ip address <x.x.x.x>",
						   "-m modbus id <1-...>",
						   "-p modbus tcp port <1-...>",
						   "-s set export limit <0-11>",
						   "-h help");
				   exit(0);
			default:   fprintf(stderr, "%s: Unknown argument, -h for help\n", argv[0]); exit(-1);
		}
	}

	/* Argument validations */
	if( strlen(dongle_ip) == 0)
	{
		fprintf(stderr, "IP address -i is mandatory argument\n");
		exit(-1);
	}

	if (debug)
	{
		printf("Modbus ID of inverter:\t%d\n", modbus_slave);
		printf("IP address of Sdongled05:\t%s\n", dongle_ip);
		printf("Modbus port of Sdongled05:\t%d\n", modbus_port);
	}

	/* Check that the IP/TCP port is reachable */
	if (!check_port(modbus_port, dongle_ip))
	{
		fprintf(stderr, "%s: The dongle IP, TCP port combination %s:%d is not reachable\n", argv[0], dongle_ip, modbus_port);
		exit(-1);
	}

	/* Validate set export limitation argument value */
	if (set_export)
	{
		errno = 0;
		n = sscanf(export_value, "%f", &apwr);
		if (errno != 0) {
			perror("sscanf");
			exit(-1);
		} else if ( n != 1)
	       	{
			fprintf(stderr, "%s: Not able to parse the export limit argument to a numeric value\n", argv[0]);
			exit(-1);
		}
		if (debug)
			printf("Export limit argument: %s\n", export_value); 
	}

	/* Initiate libModbus iand connect to SDongle05 */
	ctx = modbus_new_tcp(dongle_ip, modbus_port);
	if( debug)
		modbus_set_debug(ctx, TRUE);
	modbus_set_slave(ctx, modbus_slave);
	modbus_set_response_timeout(ctx, 1, 800000);

	if(modbus_connect(ctx) == -1)
	{
		fprintf(stderr, "%s: Unable to connect to inverter\n", argv[0]);
		return -1;
	}

	if (debug)
		printf("Connected to SUN2000\n");

	sleep(3);


	// Catch signals
	set_signal_action();

	/* Set export limit requested */
	if (set_export)
	{
		if ( apwr >= LOW_LIMIT && apwr <= HIGH_LIMIT)
		{
			if( !set_power_limit(apwr*1000))
			{
				fprintf(stderr, "%s: Unable tp set export limit value\n", argv[0]);
	modbus_terminate_and_exit(-1);
			}
		}
		sleep(2);
	}


	/* Read export limitation value */
	if (!read_active_power(&apwr))
	{
		fprintf(stderr,"%s: unable to read modbus active power object (4...)\n", argv[0]) ;
	modbus_terminate_and_exit(-1);
	}
	if( debug)
	{
		printf("apwr %f\n", apwr);
	} else {
		printf("%.2f\n", apwr);
	}

	modbus_terminate_and_exit(0);
}
