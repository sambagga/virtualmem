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
#include	<sys/types.h>
#include	<sys/time.h>
#include	<time.h>

using namespace std;
#define BUFSIZE 1024
char *progname;
vector<int> pages;
void usage();

typedef struct ll {
	int page;
	struct ll *down;
} llstack;

int avframes, npages;
extern char *optarg;
extern int optind;

FILE *inpfile_fd = NULL;
//Optimal Algorithm
int optimal() {
	int *frames = new int[avframes];
	int i = 0, j = 0, k = 0, flag;
	int pagerep = 0, pagecount = 0;
	//Add initial pages to frames
	for (; pagecount < avframes; i++) {
		flag = 0;
		for (j = 0; j < pagecount; j++) {
			// Check for references
			if (pages[i] == frames[j]) {
				flag = 1;
				break;
			}
		}
		//Add page
		if (flag == 0) {
			frames[pagecount] = pages[i];
			pagecount++;
		}
	}
	//Main algorithm
	int max, tcount, tmax = 0, flagmax = 0;
	for (; i < npages; i++) {
		flag = 0; // to track if page in frame is referenced
		for (j = 0; j < avframes; j++) {
			if (frames[j] == pages[i]) {
				flag = 1; //reference found
				break;
			}
		}

		if (flag == 0) { //no reference
			max = 0;
			tmax = 0;
			//Find the victim page that will not be used longest time in future
			for (k = 0; k < avframes; k++) {
				flagmax = 0;
				for (j = i + 1; j < npages; j++) {
					if (frames[k] == pages[j]) {
						//Compare the count of pages till reference
						flagmax = 1;
						if (tmax < j) {
							max = k;
							tmax = j;
						}
						break;
					}
				}
				//If page is never referenced in future it becomes the victim
				if (flagmax == 0) {
					max = k;
					tmax = j;
					break;
				}
			}
			frames[max] = pages[i];
			pagerep++;
		}
	}
	return pagerep;
}
//FIFO Algorithm
int fifo() {
	int *frames = new int[avframes];
	int i = 0, j = 0, flag;
	int pagerep = 0, pagecount = 0;
	//Add initial pages to frames
	for (; pagecount < avframes; i++) {
		flag = 0;
		// Check for references
		for (j = 0; j < pagecount; j++) {
			if (pages[i] == frames[j]) {
				flag = 1;
				break;
			}
		}
		//Page not referenced, find victim
		if (flag == 0) {
			frames[pagecount] = pages[i];
			pagecount++;
		}
	}
	for (; i < npages; i++) {
		flag = 0;
		//check for references
		for (j = 0; j < avframes; j++) {
			if (frames[j] == pages[i]) {
				flag = 1;
				break;
			}
		}
		//Victim is one that came first
		if (flag == 0) {
			frames[pagerep % avframes] = pages[i];
			pagerep++;
		}
	}
	return pagerep;
}
int lfu() {
	map<int, int> freq;
	int *frames = new int[avframes];
	int i = 0, j = 0, k, flag;
	int ptr = 0;
	int pagerep = 0, pagecount = 0;
	//Add initial pages to frames
	for (; pagecount < avframes; i++) {
		flag = 0;
		// Check for references
		for (j = 0; j < pagecount; j++) {
			if (pages[i] == frames[j]) {
				flag = 1;
				break;
			}
		}
		//Assign frame to new page and initialize frequency
		if (flag == 0) {
			frames[pagecount] = pages[i];
			pagecount++;
			freq[pages[i]] = 1;
		} else {//Page referenced, increase frequency
			freq[pages[i]]++;
		}
	}
	//Check for reference in frames
	for (; i < npages; i++) {
		flag = 0;
		for (j = 0; j < avframes; j++) {
			if (frames[j] == pages[i]) {
				flag = 1;
				break;
			}
		}
		int min;
		//Page not found in frame, find victim
		if (flag == 0) {
			min = ptr;
			//victim is one with min frequency
			for (j = min + 1, k = 0; k < avframes; j++, k++) {
				j = j % avframes;
				if (freq[frames[min]] > freq[frames[j]])
					min = j;
			}
			frames[min] = pages[i];
			//Page appeared before, increase frequency
			if (freq[pages[i]] != 0)
				freq[pages[i]]++;
			//New page, initialize
			else
				freq[pages[i]] = 1;
			//Move the pointer, to track first in
			ptr = (min + 1) % avframes;
			pagerep++;
		} else {
			freq[pages[i]]++;
		}
	}
	return pagerep;
}
int lrustack() {
	llstack *top = NULL, *temp1, *temp2;
	int i = 0, j = 0, flag;
	int pagerep = 0, pagecount = 0;
	//Add initial pages to frames
	for (temp1 = top; pagecount < avframes; i++) {
		flag = 0;
		// Check for references
		for (temp2 = top, j = 0; j < pagecount && temp2 != NULL; j++, temp2 =
				temp2->down) {
			if (pages[i] == temp2->page) {
				flag = 1;
				break;
			}
		}
		//Add page
		if (flag == 0) {
			temp2 = (llstack *) malloc(sizeof(llstack*));
			temp2->page = pages[i];
			temp2->down = NULL;
			pagecount++;
			if (top == NULL) {
				top = temp2;
			} else {
				temp2->down = top;
				top = temp2;
			}
		}
	}
	llstack *prev;
	//main algorithm
	for (; i < npages; i++) {
		flag = 0;
		//if referenced page is at top of stack
		if (top->page == pages[i]) {
			continue;
		} else {
			//find and remove the referenced page from stack and move to top
			for (temp1 = top; temp1 != NULL; temp1 = temp1->down) {
				if (temp1->page == pages[i]) {
					flag = 1;
					prev->down = temp1->down;
					temp1->down = top;
					top = temp1;
					break;
				}
				prev = temp1;
			}
			//if not referenced
			if (flag == 0) {
				//scan to bottom
				for (temp1 = top; temp1->down != NULL; temp1 = temp1->down) {
					prev = temp1;
				}
				// remove last page and add new page at top of stack
				prev->down = NULL;
				temp2 = (llstack *) malloc(sizeof(llstack*));
				temp2->page = pages[i];
				temp2->down = top;
				top = temp2;
				pagerep++;
			}
		}
	}
	return pagerep;
}
int lruclock() {
	int *frames = new int[avframes];
	int *clock = new int[avframes];
	int i = 0, j = 0, flag;
	int pagerep = 0, pagecount = 0;
	//Add initial pages to frames
	for (; pagecount < avframes; i++) {
		flag = 0;
		// Check for references
		for (j = 0; j < pagecount; j++) {
			if (pages[i] == frames[j]) {
				flag = 1;
				clock[j] = 1;
				break;
			}
		}
		//Add page
		if (flag == 0) {
			frames[pagecount] = pages[i];
			pagecount++;
			clock[j] = 0;
		}
	}
	int ptr = 0;
	//Check for page references
	for (; i < npages; i++) {
		flag = 0;
		for (j = 0; j < avframes; j++) {
			if (frames[j] == pages[i]) {
				flag = 1;
				clock[j] = 1;//if referenced clock set to 1
				break;
			}
		}
		//Page not found in frame
		//Find the victim
		if (flag == 0) {
			for (j = ptr;; j++) {
				j = j % avframes;
				//if clock is 0, victim found
				if (clock[j] == 0) {
					frames[j] = pages[i];
					pagerep++;
					if (j + 1 < avframes)
						ptr = j + 1;
					else
						ptr = 0;
					break;
				} else {//clock set to 0,second chance
					clock[j] = 0;
					ptr = j;
				}
			}
		}
	}
	return pagerep;
}
int lruref8() {
	map<int, int> ref8;
	map<int, int>::iterator it;
	int *frames = new int[avframes];
	int i = 0, j = 0, flag;
	int pagerep = 0, pagecount = 0;
	//Add initial pages to frames
	for (; pagecount < avframes; i++) {
		flag = 0;
		//Shift bits right
		for (j = 0; j < ref8.size(); j++)
			ref8[j] = ref8[j] >> 1;
		//Check for references
		for (j = 0; j < pagecount; j++) {
			if (pages[i] == frames[j]) {
				flag = 1;
				if (ref8[pages[i]])
					ref8[pages[i]] += 128;
				else
					ref8[pages[i]] = 128;
				break;
			}
		}
		//Add page
		if (flag == 0) {
			frames[pagecount] = pages[i];
			pagecount++;
			ref8[pages[i]] = 128;
		}
	}
	for (; i < npages; i++) {
		flag = 0;
		//Scan for references
		for (j = 0; j < avframes; j++) {
			if (frames[j] == pages[i]) {
				flag = 1;
				break;
			}
		}
		int min;
		// Find the minimum ref8 value
		if (flag == 0) {
			min = 0;
			for (j = 1; j < avframes; j++) {
				if (ref8[frames[min]] > ref8[frames[j]])
					min = j;
			}
			frames[min] = pages[i];
			// Shift bits right
			for (j = 0; j < ref8.size(); j++)
				ref8[j] = ref8[j] >> 1;
			//Referred page
			if (ref8[pages[i]])
				ref8[pages[i]] += 128;
			else
				ref8[pages[i]] = 128;
			pagerep++;
		} else {
			ref8[pages[i]] = ref8[pages[i]] >> 1;
			ref8[pages[i]] += 128;
		}
	}
	return pagerep;
}
int main(int argc, char *argv[]) {
	struct timeval optst, optend, algst, algend;
	char inpfile[256];
	char buf[BUF_LEN];
	int ifile = 0, ch;
	char algo[12];
	avframes = 5;
	int pgrep = 1;
	//check various options
	while ((ch = getopt(argc, argv, "hr:f:i:")) != -1)
		switch (ch) {
		case 'r': //set scheduling policy
			strcpy(algo, optarg);
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
			else{
				cout<<"\nInvalid Policy!";
				usage();
			}
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
	if(avframes<1)
	{
		cout<<"\nInvalid no of frames!";
		usage();
	}
	int i = 0, temp;
	//Open the file to get pages sequence
	if (ifile == 1) {
		inpfile_fd = fopen(inpfile, "r");
		while (!feof(inpfile_fd)) {
			fscanf(inpfile_fd, "%d", &temp);
			pages.push_back(temp);
			i++;
		}
		npages = i;
		fclose(inpfile_fd);
	} else {//Take input from stdin for pages sequence
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
	//Display the result
	gettimeofday(&optst, NULL);
	int optrep = optimal();
	gettimeofday(&optend, NULL);
	int algrep;
	if (pgrep == 1) {
		gettimeofday(&algst, NULL);
		algrep = fifo();
		gettimeofday(&algend, NULL);
	} else if (pgrep == 2) {
		gettimeofday(&algst, NULL);
		algrep = lfu();
		gettimeofday(&algend, NULL);
	} else if (pgrep == 3) {
		gettimeofday(&algst, NULL);
		algrep = lrustack();
		gettimeofday(&algend, NULL);
	} else if (pgrep == 4) {
		gettimeofday(&algst, NULL);
		algrep = lruclock();
		gettimeofday(&algend, NULL);
	} else if (pgrep == 5) {
		gettimeofday(&algst, NULL);
		algrep = lruref8();
		gettimeofday(&algend, NULL);
	}
	cout << "# of page replacements with " << algo << "\t:" << algrep;
	cout << "\n# of page replacements with Optimal\t:" << optrep;
	cout << "\n% page replacement penalty using LFU\t:"
			<< ((float) (algrep - optrep) / optrep) * 100;
	uint algtime = (uint) algend.tv_usec - (uint) algst.tv_usec;
	uint opttime = (uint) optend.tv_usec - (uint) optst.tv_usec;
	cout << "\nTotal time to run " << algo << " algorithm\t:" << algtime
			<< "usec";
	cout << "\nTotal time to run Optimal algorithm\t:" << opttime << "usec";
	if (algtime < opttime)
		cout << "\n" << algo << " is "
				<< ((float) (opttime - algtime) / algtime) * 100
				<< "% faster than Optimal algorithm.";
	else
		cout << "\n" << algo << " is "
				<< ((float) (algtime - opttime) / opttime) * 100
				<< "% slower than Optimal algorithm.";
	return (0);
}

//print usage string and exit

void usage() {
	printf(
			"usage:virtualmem [−h] [-f available-frames] [−r replacement-policy] [−i input_file]\n");
	printf(
			"−h : Print a usage summary with all options and exit."
					"-f available-frames : Set the number of available frames(>0). By default it should be 5."
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
