virtualmem
==========

A Simple Virtual Memory Manager

To run virtualmem:
1. Compile the code using ./makefile
2. Modify the execute file according to :
   virtualmem [−h] [-f available-frames] [−r replacement-policy] [−i input_file]
   −h : Print a usage summary with all options and exit
   -f available-frames : Set the number of available frames(>0). By default it should be 5
   −r replacement-policy : Set the page replacement policy. It can be either FIFO (First-in-first-out
  							LFU (Least-frequently-used)
								LRU-STACK (Least-recently-used stack implementation)
								LRU-CLOCK ((Least-recently-used clock implementation –second chance alg.)
								LRU-REF8 (Least-recently-used Reference-bit Implementation, using 8 reference bits)
3. Execute the code using ./execute.
4. By default iput paging sequence is read from stdin unless file is specified.