#define	BUF_LEN	8192
#include	<iostream>
#include	<vector>
#include	<map>
#include	<stdio.h>
#include 	<syslog.h>
#include	<unistd.h>
#include	<dirent.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include 	<termios.h>
#include	<fcntl.h>
#include	<assert.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<netinet/in.h>
#include	<inttypes.h>
#include	<pthread.h>
#include	<semaphore.h>
#include	<time.h>
#include 	<arpa/inet.h>
#include 	<sys/stat.h>

using namespace std;
#define BUFSIZE 1024
char *progname;
char buf[BUF_LEN];
char inpfile[256];
void usage();

int ch,ifile,avframes,sch;
extern char *optarg;
extern int optind;

FILE *inpfile_fd = NULL;

int main(int argc, char *argv[]) {
	inpfile[0] = '\0';
	ifile = 0;
	avframes=5;
	//check various options
	while ((ch = getopt(argc, argv, "hr:f:i:")) != -1)
		switch (ch) {
		case 'r': //set scheduling policy
			if (strcmp(optarg, "FIFO"))
				sch = 1;
			else if (strcmp(optarg, "SJF"))
				sch = 2;
			break;
		case 'f': //no of frames
			avframes = atoi(optarg);
			break;
		case 'i': //address of log file
			ifile=1;
			strncpy(inpfile, optarg, sizeof(inpfile));
			break;
		case 'h': //help
		default:
			usage();
			break;
		}
	argc -= optind;
	if (argc != 0)
		usage();

	return (0);
}


//print usage string and exit

void usage() {
	printf("usage:virtualmem [−h] [-f available-frames] [−r replacement-policy] [−i input_file]\n");
	printf("−h : Print a usage summary with all options and exit."
			"-f available-frames : Set the number of available frames. By default it should be 5."
			"−r replacement-policy : Set the page replacement policy. It can be either"
			"FIFO (First-in-first-out)"
			"LFU (Least-frequently-used)"
			"LRU-STACK (Least-recently-used stack implementation)"
			"LRU-CLOCK ((Least-recently-used clock implementation –second chance alg.)."
			"LRU-REF8 (Least-recently-used Reference-bit Implementation, using 8 reference bits)"
			"The default will be FIFO."
			"−i input file : Read the page reference sequence from a specified file. If not given,the sequence should be read from STDIN (ended with ENTER).\n");
		exit(1);
}
