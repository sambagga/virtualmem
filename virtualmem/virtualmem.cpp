#define	BUF_LEN	8192
#include	<iostream>
#include	<vector>
#include	<map>
#include	<stdio.h>
#include 	<syslog.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<assert.h>
#include	<sys/types.h>
#include	<time.h>

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
//Optimal Algorithm
void optimal() {
	int *frames = new int[avframes];
	int i = 0, j = 0, k = 0, flag;
	pagerep = 0;
	//Add intitial pages to frames
	for (; j < avframes; i++) {
		flag = 0;
		for (j = 0; j < i; j++) {
			// Check for references
			if (pages[i] == frames[j]) {
				flag = 1;
				break;
			}
		}
		//Add page
		if (flag == 0) {
			frames[j] = pages[i];
		}
	}
	//Main algorithm
	for (; i < npages; i++) {
		flag = 0;// to track if page in frame is referenced
		for (j = 0; j < avframes; j++) {
			if (frames[j] == pages[i]) {
				flag = 1; //reference found
				break;
			}
		}
		int max, tcount, tmax = 0, flagmax = 0;
		if (flag == 0) {//no reference
			max = 0;
			//Find the victim page that will not be used longest time in future
			for (k = 0; k < avframes; k++) {
				flagmax = 0;
				for (j = i + 1, tcount = 1; j < npages; j++, tcount++) {
					if (frames[k] == pages[j]) {
						//Compare the count of pages till reference
						if (tmax < tcount) {
							flagmax = 1;
							max = k;
							tmax=tcount;
							break;
						}
					}
				}
				//If page is never referenced in future it becomes the victim
				if (flagmax == 0) {
					max = k;
					break;
				}
			}
			frames[max] = pages[i];
			pagerep++;
		}
	}
	cout << "\n" << pagerep;
}
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
			min = 0;
			for (j = 1; j < avframes; j++) {
				if (freq[frames[min]] > freq[frames[j]])
					min = j;
			}
			frames[min] = pages[i];
			if (freq[pages[i]] != 0)
				freq[pages[i]]++;
			else
				freq[pages[i]] = 1;
			pagerep++;
		} else {
			freq[pages[i]]++;
		}
	}
	cout << "\n" << pagerep;
}
void lruref8() {
	map<int, int> ref8;
	map<int, int>::iterator it;
	int *frames = new int[avframes];
	int i = 0, j = 0, flag;
	pagerep = 0;
	for (; j < avframes; i++) {
		for (it = ref8.begin(); it != ref8.end(); it++)
			(*it).second = (*it).second >> 1;
		for (j = 0; j < i; j++) {
			if (pages[i] == frames[j]) {
				if (ref8[pages[i]])
					ref8[pages[i]] += 128;
				else
					ref8[pages[i]] = 128;
			}
		}
	}
	for (; i < npages; i++) {
		flag = 0;
		for (it = ref8.begin(); it != ref8.end(); it++)
			(*it).second = (*it).second >> 1;
		for (j = 0; j < avframes; j++) {
			if (frames[j] == pages[i]) {
				flag = 1;
				break;
			}
		}
		int min;
		if (flag == 0) {
			min = 0;
			for (j = 1; j < avframes; j++) {
				if (ref8[frames[min]] > ref8[frames[j]])
					min = j;
			}
			frames[min] = pages[i];
			if (ref8[pages[i]])
				ref8[pages[i]] += 128;
			else
				ref8[pages[i]] = 128;
			pagerep++;
		} else {
			ref8[pages[i]] += 128;
		}
	}
	cout << "\n" << pagerep;
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
			if (strcmp(optarg, "FIFO") == 0)
				pgrep = 1;
			else if (strcmp(optarg, "LFU") == 0)
				pgrep = 2;
			else if (strcmp(optarg, "LRU-STACK") == 0)
				pgrep = 3;
			else if (strcmp(optarg, "LRU-CLOCK") == 0)
				pgrep = 4;
			else if (strcmp(optarg, "LRU-REF8") == 0)
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
		while (!feof(inpfile_fd)) {
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
	cout << "Optimal:\n";
	optimal();
	if (pgrep == 1) {
		cout << "FIFO:\n";
		fifo();
	} else if (pgrep == 2) {
		cout << "LFU:\n";
		lfu();
	}
	/*else if(pgrep==3)
	 lrustack();
	 else if(pgrep==4)
	 lruclock();*/
	else if (pgrep == 5) {
		cout << "LRU-REF8\n";
		lruref8();
	}
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
