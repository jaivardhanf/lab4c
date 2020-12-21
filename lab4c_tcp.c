//NAME: Jai Vardhan Fatehpuria
//EMAIL: jaivardhan.f@gmail.com
//ID: 804817305

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <ctype.h>
#include <mraa.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <fcntl.h>

int period = 1;
int scale = 'F';
int opt_log = 0;
int stop = 0;

sig_atomic_t volatile run_flag = 1;

mraa_aio_context temp_sensor;
mraa_gpio_context button;

struct timeval current_time;
struct tm* time_info;

const int B = 4275;               //Value of thermistor
const int R0 = 100000;

char* log_file;
int log_fd;

char* id = "";
char* host = "";
int portno;

int sock_fd;

void terminate()
{
	time_info = localtime(&current_time.tv_sec);
    run_flag=0;
	char report[100];
	sprintf(report, "%02d:%02d:%02d SHUTDOWN\n", time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
	int writelength = strlen(report);
	if (write(sock_fd, report, writelength) < 0)
	{
		fprintf(stderr, "Error : %s\n", strerror(errno));
		exit(1);
	}
	if (opt_log)
	{
		if (write(log_fd, report, writelength) < 0)
		{
			fprintf(stderr, "Error : %s\n", strerror(errno)); //cant write to log file
			exit(1);
		}
	}
	exit(0);
}

float get_temp()
{
    int value = mraa_aio_read(temp_sensor);
    float R = 1023.0/value-1.0;
	R = R0*R;
	float ctemp = 1.0/(log(R/R0)/B+1/298.15)-273.15;
	if(scale=='F')
	{
		float ftemp = (ctemp*9)/5 + 32;
		return ftemp;
	}
	else
	{
		return ctemp;
	}
	
}




void exec_command(const char* command)
{
	if(opt_log)
	{
		int writelength = strlen(command);
		if (write(log_fd, command, writelength) < 0)
		{
			fprintf(stderr, "Error: %s\n", strerror(errno)); // cant write to logfile
			exit(1);
		}
		if (write(log_fd, "\n", 1) < 0)
		{
			fprintf(stderr, "Error: %s\n", strerror(errno));
			exit(1);
		}
	}
	if (strcmp(command, "SCALE=C") == 0)
	{
		scale = 'C';
	}
	else if (strcmp(command, "SCALE=F") == 0)
	{
		scale = 'F';
	}
	else if (strcmp(command, "START") == 0)
	{
		stop = 0;
	}
	else if (strcmp(command, "STOP") == 0)
	{
		stop = 1;
	}
	else if (strncmp(command, "LOG", 3) == 0)
	{

	}
	else if (strcmp(command, "OFF") == 0)
	{
		terminate();
	}
	else if (strncmp(command, "PERIOD=", 7) == 0)
	{
			period = atoi(command + 7);
	}
}

//Used for debugging. Please ignore.

void printArray(int A[], int size) {
    int i;
    for (i=0; i < size; i++)
        printf("%d ", A[i]);
    printf("\n");
}

void error_occured()
{
    fprintf(stderr, "Error has occured. \n");
    
}

int main(int argc, char* argv[]) {


	struct option options[] = {
		{"period", required_argument, NULL, 'p'},
		{"scale", required_argument, NULL, 's'},
		{"log", required_argument, NULL, 'l'},
        {"id", required_argument, NULL, 'i'},
        {"host", required_argument, NULL, 'h'},
		{0, 0, 0, 0}
	};

	int opt;
	while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
		switch (opt) {
			case 's':
                if (optarg[0] == 'C')
                    scale = 'C';
                break;
			case 'p':
                period = atoi(optarg);
                break;
            case 'l':
                opt_log = 1;
				log_file = optarg;
                break;
            case 'i':
                id = optarg;
                break;
                host = optarg;
                break;
            case 'h':
                host = optarg;
                break;
			default:
				fprintf(stderr, "Invalid arguments\n");
				exit(1);
		}
	}

    if(optind >=argc)
    {
        fprintf(stderr, "no port number\n");
        exit(1);
    }

    portno = atoi(argv[optind]);
    if(portno <=0)
    {
        fprintf(stderr, "invalid port number\n");
        exit(1);
    }

    if(strlen(id)!=9)
    {
        fprintf(stderr, "invalid ID number\n");
        exit(1);
    }

    if(opt_log==0)
    {
        fprintf(stderr, "no log file provided\n");
        exit(1);
    }

    if(strlen(host)==0)
    {
        fprintf(stderr, "no host name/address provided\n");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    struct hostent *server;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno)); // socket cant be created
        exit(1);
    }
    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "No such host: %s\n", strerror(errno));
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sock_fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "Error connecting: %s\n", strerror(errno));
        exit(1);
    }

    char idbuffer[15];
	sprintf(idbuffer, "ID=%s\n", id);
	if (write(sock_fd, idbuffer, strlen(idbuffer)) < 0)
    {
		fprintf(stderr, "Error writing ID number to server: %s\n", strerror(errno));
        exit(1);
    }


    temp_sensor = mraa_aio_init(1);

	gettimeofday(&current_time, NULL);
	time_t nexttime;
	nexttime = current_time.tv_sec + period;



	if(opt_log)
	{
		log_fd = creat(log_file, 0666);
		if(log_fd<0)
		{
			fprintf(stderr, "Error creating log file: %s\n", strerror(errno));
			exit(1);
		}
	}
	struct pollfd polls[1];
	polls[0].fd = sock_fd;
    polls[0].events = POLLIN;

	char inputbuffer[32];
	char command[32];
	int current = 0;

    while(run_flag) {
        gettimeofday(&current_time, NULL);
		if (current_time.tv_sec >= nexttime)
		{
			if (!stop)
			{
				float temperature = get_temp();
				nexttime = current_time.tv_sec + period;
				time_info = localtime(&current_time.tv_sec);
				char report[100];
				sprintf(report, "%02d:%02d:%02d %0.1f\n", time_info->tm_hour, time_info->tm_min, time_info->tm_sec, temperature);
				int writelength = strlen(report);
				if (write(sock_fd, report, writelength) < 0)
				{
					fprintf(stderr, "Error writing to std out: %s\n", strerror(errno));
					exit(1);
				}
				if (opt_log)
				{
					if (write(log_fd, report, writelength) < 0)
					{
						fprintf(stderr, "Error writing to log file: %s\n", strerror(errno));
						exit(1);
					}
				}
			}
		}
        int ret = poll(polls, 1, 0);

		if (ret<0)
		{
			fprintf(stderr, "Polling error: %s\n", strerror(errno));
			exit(1);
		}
		else if (ret>0)
        {
			if (polls[0].revents & POLLIN)
			{
				int read_length = read(sock_fd, &inputbuffer, 32);
				if (read_length < 0)
				{
					fprintf(stderr, "Read error: %s\n", strerror(errno));
					exit(1);
				}
                int i = 0;
                while (i < read_length)
                {
                    if (inputbuffer[i] == '\n')
                    {
                        command[current] = '\0';
                        exec_command(command);
                        current = 0;
                    }
                    else
                    {
                        command[current] = inputbuffer[i];
                        current++;
                    }
                    i++;
                }
			}
		}
	}
    mraa_aio_close(temp_sensor);
    exit(0);
}
