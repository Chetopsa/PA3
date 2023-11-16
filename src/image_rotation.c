#include "image_rotation.h"

 
 
//Global integer to indicate the length of the queue??
int queue_size = 0;
request_t *queue;
//Global integer to indicate the number of worker threads
int num_workers;
//Global file pointer for writing to log file in worker??
FILE *log_file;
//Might be helpful to track the ID's of your threads in a global array
int thread_ids[MAX_THREADS];

//What kind of locks will you need to make everything thread safe? [Hint you need multiple]
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER; // Mutex for queue access
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER; // producer cond
pthread_mutex_t processing_wait = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t processing_wait_cond = PTHREAD_COND_INITIALIZER; 

char output_folder[PATH_MAX];
int no_files = 0; //signal when workers should stop; 1 means no_files left
//How will you track the requests globally between threads? How will you ensure this is thread safe?
//How will you track which index in the request queue to remove next?
//How will you update and utilize the current number of requests in the request queue?
//How will you track the p_thread's that you create for workers?
pthread_t workers[MAX_THREADS];
//How will you know where to insert the next request received into the request queue?

//added functions to maintain a queue
/*
    initializes a queue struct mallocs it
    returns a request_t pointer
*/

request_t* queue_init(){
    request_t *queue = (request_t *)malloc(sizeof(request_t));
    queue->head = NULL;
}
/* 
    pushes a node to the the queue takes the two data sttributes
    allocates a node then pushes onto the queue, returns 1 if succesful
*/
int push(request_t *queue, char* file_name, int rotation_angle) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    strcpy(new_node->file_name, file_name);
    new_node->rotation_angle = rotation_angle;
    new_node->next = queue->head;
    queue->head = new_node;
    queue_size++;
    return 1;
}

/* takes in a request_t struct as an agrument and returns
    a node, updates the size and head attributes */
node_t* pop(request_t *queue) {
    if (queue_size == 0) {
        return NULL;
    }
    node_t *temp = queue->head;
    queue->head = queue->head->next;
    queue_size--;
    return temp;
}
/* freees all remaing node structs in the quue as well as the quee
    struct itself */
int free_q(request_t *queue){
    node_t* cur = queue->head;
    while(cur != NULL){
        node_t* temp = cur->next;
        free(cur);
        cur = temp;
    }
    free(queue);
    return 1;
}

/*
    The Function takes:
    to_write: A file pointer of where to write the logs. 
    requestNumber: the request number that the thread just finished.
    file_name: the name of the file that just got processed. 

    The function output: 
    it should output the threadId, requestNumber, file_name into the logfile and stdout.
*/
void log_pretty_print(FILE* to_write, int threadId, int requestNumber, char * file_name){
   // Lock the log file mutex before writing to the log
    // Write to the log file and to stdout
    fprintf(to_write, "[%d][%d][%s]\n", threadId, requestNumber, file_name);
    printf("[%d][%d][%s]\n", threadId, requestNumber, file_name);
    // Unlock the log file mutex after writing
}


/*

    1: The processing function takes a void* argument called args. It is expected to be a pointer to a structure processing_args_t 
    that contains information necessary for processing.

    2: The processing thread need to traverse a given dictionary and add its files into the shared queue while maintaining synchronization using lock and unlock. 

    3: The processing thread should pthread_cond_signal/broadcast once it finish the traversing to wake the worker up from their wait.

    4: The processing thread will block(pthread_cond_wait) for a condition variable until the workers are done with the processing of the requests and the queue is empty.

    5: The processing thread will cross check if the condition from step 4 is met and it will signal to the worker to exit and it will exit.

*/

void *processing(void *args)
{
    processing_args_t *p_args = (processing_args_t *)args;
    pthread_mutex_lock(&queue_lock);

    printf("Processing QUEUE_LOCKED\n");
    DIR *dir;
    struct dirent *entry;
    // open current directory
    if ((dir = opendir(p_args->inp_file)) < 0){
        perror("error opening directory\n");
        exit(1);
    }
    // Traverse through all entries
    while((entry = readdir(dir)) != NULL){
        // skip . and ..
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        char* extention = strchr(entry->d_name, '.');
        if(strcmp(extention, ".png") == 0){
            char entry_path[PATH_MAX];
            sprintf(entry_path, "%s/%s", p_args->inp_file, entry->d_name);
            push(queue, entry_path, p_args->rotation_angle); 
        }
    }

    no_files = 1;
    pthread_mutex_unlock(&queue_lock);
    printf("Processing QUEUE_UNLOCKED\n");
    pthread_cond_broadcast(&queue_not_empty); 
    printf("Processing SIGNAL_QUEUE_NOT_EMPTY\n");
    // close current directory

    closedir(dir);
    pthread_exit(NULL);
}

/*
    1: The worker threads takes an int ID as a parameter

    2: The Worker thread will block(pthread_cond_wait) for a condition variable that there is a requests in the queue. 

    3: The Worker threads will also block(pthread_cond_wait) once the queue is empty and wait for a signal to either exit or do work.

    4: The Worker thread will processes request from the queue while maintaining synchronization using lock and unlock. 

    5: The worker thread will write the data back to the given output dir as passed in main. 

    6:  The Worker thread will log the request from the queue while maintaining synchronization using lock and unlock.  

    8: Hint the worker thread should be in a While(1) loop since a worker thread can process multiple requests and It will have two while loops in total
        that is just a recommendation feel free to implement it your way :) 
    9: You may need different lock depending on the job.  

*/


void * worker(void *args)
{
    int request_num = 1;
    while(1){
        //inter submission
        // if ((fprintf(stderr, "threadID: %d\n", *id)) < 0){
        //     perror("error writing to file\n");
        //     exit(1);
        // }
            int* id = (int*) args;
            pthread_mutex_lock(&queue_lock);
            printf("worker%d locked\n", *id);
            while(queue_size == 0 ){
                if(no_files){
                // unlock the stand
                    pthread_mutex_unlock(&queue_lock);
                    printf("worker%d unlocked and terminating...\n", *id);
                    pthread_exit(NULL);
            }
                pthread_cond_wait(&queue_not_empty, &queue_lock);
            }    
        
            node_t* n_img = pop(queue);
            //get correct thread id

            pthread_mutex_unlock(&queue_lock);
            printf("worker%d unlocked\n", *id);
            char file_name[PATH_MAX];
            strcpy(file_name, n_img->file_name);
            //print out log
            log_pretty_print(log_file, *id, request_num++, file_name);
            int rotation_angle = n_img->rotation_angle;
             /*
            Stbi_load takes:
                A file name, int pointer for width, height, and bpp
            */
            printf("ayyy?");
            int width; int height; int bpp;
            printf("file_name  =  %s\n", file_name);
            char* image_name = get_filename_from_path(file_name);
            char output_image[PATH_MAX];
            sprintf(output_image, "%s/%s", output_folder, image_name);
            uint8_t* image_result = (uint8_t **)malloc(sizeof(uint8_t*));
            image_result = stbi_load(file_name, &width, &height, &bpp, CHANNEL_NUM);

            uint8_t **result_matrix = (uint8_t **)malloc(sizeof(uint8_t*) * width);
            uint8_t** img_matrix = (uint8_t **)malloc(sizeof(uint8_t*) * width);
            for(int i = 0; i < width; i++){
                result_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
                img_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
            }
            /*
            linear_to_image takes: 
                The image_result matrix from stbi_load
                An image matrix
                Width and height that were passed into stbi_load
            
            */
            linear_to_image(image_result, img_matrix, width, height);
            ////TODO: you should be ready to call flip_left_to_right or flip_upside_down depends on the angle(Should just be 180 or 270)
            //both take image matrix from linear_to_image, and result_matrix to store data, and width and height.
            //Hint figure out which function you will call. 
            if(rotation_angle == 180){
                printf("180 angle\n");
                flip_left_to_right(img_matrix, result_matrix, width, height);
            }
            else{ 
                printf("270 angle\n");
                flip_upside_down(img_matrix, result_matrix, width, height);
            }

            uint8_t* img_array = (uint8_t*)malloc(sizeof(uint8_t) * width * height); ///Hint malloc using sizeof(uint8_t) * width * height

            ///TODO: you should be ready to call flatten_mat function, using result_matrix
            //img_arry and width and height; 
            flatten_mat(result_matrix, img_array, width, height);

            ///TODO: You should be ready to call stbi_write_png using:
            //New path to where you wanna save the file,
            printf("file_name: %s\n", file_name);
            //Width
            //height
            //img_array
            //width*CHANNEL_NUM
            printf("output_image: %s\n", output_image);
            if (img_array == NULL){
                printf("NOT GOOOD\n\n\n\n");
            }
            printf("%d   %d\n\n", width, height );
            if(stbi_write_png(output_image, width, height, CHANNEL_NUM, img_array, width * CHANNEL_NUM) == 0){
                perror("image failed to write");
                exit(1);
            }
            free(img_array);
            free(image_result);
            for(int i = 0; i < width; i++){
                free(result_matrix[i]);
                free(img_matrix[i]);
            }
            free(result_matrix);
            free(img_matrix);
    }
}

/*
    Main:
        Get the data you need from the command line argument 
        Open the logfile
        Create the threads needed
        Join on the created threads
        Clean any data if needed. 
*/
int main(int argc, char* argv[])
{
    if(argc != 5)
    {
        fprintf(stderr, "Usage: File Path to image dirctory, File path to output dirctory, number of worker thread, and Rotation angle\n");
        exit(1);
    }
    printf("start:\n");
    //open log file
    strcpy(output_folder, argv[2]);
    log_file = fopen(LOG_FILE_NAME, "a");
    
    processing_args_t process_args;
    //create queue
    queue = queue_init();
    num_workers = atoi(argv[3]);
    
    strcpy(process_args.inp_file, argv[1]);
    process_args.rotation_angle = atoi(argv[4]);
    pthread_t process_thread;
    
    if ((pthread_create(&process_thread, NULL, (void *)processing, (void *)&process_args)) < 0) {
        perror("error creating processing thread\n");
        exit(1);
    }
    for(int i = 0; i < num_workers; i++){
        // create thread
        thread_ids[i] = i;
        if((pthread_create(&workers[i], NULL, (void *)worker, &thread_ids[i])) < 0){
            perror("error creating worker thread\n");
            exit(1);
        }
    }
    //join workers array
    for(int i = 0; i < num_workers; i++){
        if((pthread_join(workers[i], NULL)) < 0){
            perror("error joining threads\n");
            exit(1);
        }
    }
    //join proccessing thread
    if ((pthread_join(process_thread, NULL)) < 0){
        perror("error joining threads\n");
        exit(1);
    }
    printf("SUCCESSFULLY EXITS THREADS: queue_size = %d\n", queue_size);
    free_q(queue);
}