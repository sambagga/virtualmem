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
vector<int> pages;
void usage();

int ch, ifile, avframes, pgrep, npages;
int pagerep;
extern char *optarg;
extern int optind;

FILE *inpfile_fd = NULL;
void fifo() {
	int *frames = new int[avframes];
	int i = 0, j = 0, flag;
	pagerep = 0;
	for (; j < avframes; i++) {
		flag = 0;
		for (j = 0; j < i; j++) {
			if (pages[i] == frames[j]) {
				flag = 1;
				break;
			}
		}
		if (flag == 0) {
			frames[j] = pages[i];
		}
	}
	for (; i < npages; i++) {
		flag = 0;
		for (j = 0; j < avframes; j++) {
			if (frames[j] == pages[i]) {
				flag = 1;
				break;
			}
		}
		if (flag == 0) {
			frames[pagerep % avframes] = pages[i];
			pagerep++;
		}
	}
	cout << "\n" << pagerep;
}
void lfu() {
	map<int, int> freq;
	int *frames = new int[avframes];
	int i = 0, j = 0, flag;
	pagerep = 0;
	for (; j < avframes; i++) {
		flag = 0;
		for (j = 0; j < i; j++) {
			if (pages[i] == frames[j]) {
				flag = 1;
				break;
			}
		}
		if (flag == 0) {
			frames[j] = pages[i];
			freq[pages[i]] = 1;
		} else {
			freq[pages[i]]++;
		}
	}
	for (; i < npages; i++) {
		flag = 0;
		for (j = 0; j < avframes; j++) {
			if (frames[j] == pages[i]) {
				flag = 1;
				break;
			}
		}
		int min;
		if (flag == 0) {
			min=0;
			for (j = 1; j < avframes; j++) {
				if(freq[frames[min]]>freq[frames[j]])
					min=j;
			}
			frames[min]=pages[i];
			if(freq[pages[i]]!=0)
				freq[pages[i]]++;
			else
				freq[pages[i]]=1;
			pagerep++;
		}
		else{
			freq[pages[i]]++;
		}
	}
	cout<<"\n"<<pagerep;
}

int main(int argc, char *argv[]) {
	inpfile[0] = '\0';
	ifile = 0;
	avframes = 5;
	pgrep = 1;
	//check various options
	while ((ch = getopt(argc, argv, "hr:f:i:")) != -1)
		switch (ch) {
		case 'r': //set scheduling policy
			if (strcmp(optarg, "FIFO")==0)
				pgrep = 1;
			else if (strcmp(optarg, "LFU")==0)
				pgrep = 2;
			else if (strcmp(optarg, "LRU-STACK")==0)
				pgrep = 3;
			else if (strcmp(optarg, "LRU-CLOCK")==0)
				pgrep = 4;
			else if (strcmp(optarg, "LRU-REF8")==0)
				pgrep = 5;
			break;
		case 'f': //no of frames
			avframes = atoi(optarg);
			break;
		case 'i': //address of log file
			ifile = 1;
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
	int i = 0, temp;
	if (ifile == 1) {
		inpfile_fd = fopen(inpfile, "r");
		while (inpfile_fd) {
			fscanf(inpfile_fd, "%d", &temp);
			pages.push_back(temp);
			i++;
		}
		npages = i;
		fclose(inpfile_fd);
	} else {
		char *tstr;
		gets(buf);
		i = 0;
		tstr = strtok(buf, " ");
		while (tstr != NULL) {
			temp = atoi(tstr);
			pages.push_back(temp);
			i++;
			tstr = strtok(NULL, " ");
		}
		npages = i;
	}
	cout << "Pages:\n";
	for (i = 0; i < npages; i++) {
		cout << pages[i] << " ";
	}
	if (pgrep == 1)
	{
		cout<<"FIFO:\n";
		fifo();
	}
	else if (pgrep == 2)
	{
		cout<<"LFU:\n";
		lfu();
	}
	/*else if(pgrep==3)
	 lrustack();
	 else if(pgrep==4)
	 lruclock();
	 else if(pgrep==5)
	 lruref8();*/
	return (0);
}

//print usage string and exit

void usage() {
	printf(
			"usage:virtualmem [−h] [-f available-frames] [−r replacement-policy] [−i input_file]\n");
	printf(
			"−h : Print a usage summary with all options and exit."
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
