# CSCI 4061: Parallel image processing

**Project Group:** 128

**Group Members:**
- Cheston Opsasnick (opsas002)
- Ahmed Kadar (kadar008)
- Ahmed-dahir Dayib (dayib007)

**CSELabs Computer Used for Testing:** login03.cselabs.umn.edu

# Changes
**image_rotation** added functions for managing a queue
**Makefile** added zip function

# Contributions
Ahmed-dahir Dayib - helped write main and processing for inter, thread logging
Ahmed Kadar - Helped write main and processing for inter, will work on worker
Cheston Opsasnick - wrote the queues and Readme, debugged mutexes and worker

# implementing thread processing
main:
    Create the threads for the workers and single thread for proccessing
    that adds items to the queue. Joins thread when finished
processing:
    Traverse directory and add the .png files to a queue which can be accessed
    by the worker threads. To allow for thread safety use mutex locks and cvs whenever
    adding to the queue. and signal to the wokrers when no more files are left.
worker:
    Uses cv and mutex lock around the shared queue and other shared data like int no_files to
    perform actions to rotate the image and write to the output folder.
log:
    Write the thread id's to a file with with how mnay times each file executed

final: same implemetnaiton as pseudo code for inter.



    
