# CSCI 4061: Parallel image processing

**Project Group:** 128

**Group Members:**
- Cheston Opsasnick (opsas002)
- Ahmed Kadar (kadar008)
- Ahmed-dahir Dayib (dayib007)

**CSELabs Computer Used for Testing:** login03.cselabs.umn.edu

# Changes
**image_rotation** added functions for managing a queue

# Contributions
Ahmed-dahir Dayib - helped write main and processing for inter, will work on thread logging
Ahmed Kadar - Helped write main and processing for inter, will work on worker
Cheston Opsasnick - wrote the queues and Readme, will work on Worker

# implementing thread processing
main:
    Create the threads for the workers and single thread for proccessing
    that adds items to the queue.
processing:
    Traverse directory and add the .png files to a queue which can be accessed
    by the worker threads. To allow for thread safety use mutex locks and cvs whenever
    adding to the queue.
worker:
    uses cv and mutex lock around the shared queue and other shared data like the thread_list to access it, then
    perform actions to rotate the image and write to the output folder.
log:
    writes the threads to a file, uses mutex locks to ensure no thread lcokign or race conditions when writing to file



    
