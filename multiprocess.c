#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include<sys/errno.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>

#define MSGSZ 1024

/* We must define union semun ourselves. */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
};

// Message Buffer structure definition
typedef struct my_msgbuf {
    long    mtype;
    char    mtext[MSGSZ];
} message_buf;

// Structure of the shared memory 
struct shared_mem_struct{
    int sem_id_queue_not_busy;
    // key_t sem_key;
    // int sem_flag;
    // struct msqid_ds sys_buffer;
    unsigned long produced_request;
    unsigned long processed_request;
    unsigned long producer_block;
    unsigned long consumer_idle;
    double total_block_t;
    int buffer_index_write;
    int buffer_index_read;
    int buffer_msg_count;
    int buffer_msg_sizes[MSGSZ];
};



// function prototypes
int create_producer_consumer(int P_n, int C_n);
void produce();
void consume();
void performance_stats (int status_interval_t);
useconds_t get_producer_delay(double P_t_param);
useconds_t get_consumer_delay(double prob);
int get_msg_size(int req_size);
double normal_distribution (double sigma);
void sighandler(int signum);
int binary_semaphore_allocation (key_t key, int sem_flags);
int binary_semaphore_deallocate (int semid);
int binary_semaphore_initialize (int semid);
int binary_semaphore_wait (int semid);
int binary_semaphore_post (int semid);


// variables to capture the command line arguments
int TOTAL_RUN_TIME;
int B_BUFFER_SIZE;
int P_COUNT;
int C_COUNT;
float P_t_PRODUCER_DELAY_DIST_PARAM;
float R_s_REQUEST_SIZE_DIST_PARAM;
float C_t1_CONSUMED_WITH_IO_ONLY_DIST_PARAM;
float C_t2_CONSUMED_WITH_IO_DISC_DB_DIST_PARAM;
float P_i_PROBABILITY_OF_C_t1;

// SIGNAL
sig_atomic_t quit = 0;

// SHARED MEMOMRY
int shm_id;
key_t shm_key = 3000;
int shm_flag  = IPC_CREAT | 0666;
void *shared_mem_temp_ptr  = (void *)0;
struct shared_mem_struct *shared_mem;

// BINARY SEMAPHORE
// int sem_id_queue_not_busy;
// key_t sem_key = 2222;
// int sem_flag  = IPC_CREAT | 0666;

// MESSAGE QUEUE
int msqid; 
key_t msqkey  = 4000;              
int msqflg = IPC_CREAT | 0666;  // to create queue and make it read and appendable by all


int main (int argc,char *argv[]) {
    if (argc != 10){
        fprintf (stderr, "INCORRECT EXECUTION COMMAND \n");
        fprintf (stderr, "USAGE: ./<object_name> <T> <B> <P> <C> <P_t param> <R_s param> <C_t1 param> <C_t2 param> <p_i>\n");
        exit (1);
    }
    
    // TIME CALCULATION VARIABLES
    struct timeval execution_start_time;
    struct timeval execution_end_time;
    struct timeval total_execution_time;
    double total_execution_t            =0.0;
    double total_block_t                =0.0;
    double average_block_t              =0.0;
    double average_percentage_block_t   =0.0;
    double percentage_req_processed     =0.0;
    double percentage_req_blocked       =0.0;
          
    // capturing the command line arguments
    TOTAL_RUN_TIME                              = atoi(argv[1]);
    B_BUFFER_SIZE                               = atoi(argv[2]);
    P_COUNT                                     = atoi(argv[3]);
    C_COUNT                                     = atoi(argv[4]);
    P_t_PRODUCER_DELAY_DIST_PARAM               = atoi(argv[5]);
    R_s_REQUEST_SIZE_DIST_PARAM                 = atoi(argv[6]);
    C_t1_CONSUMED_WITH_IO_ONLY_DIST_PARAM       = atoi(argv[7]);
    C_t2_CONSUMED_WITH_IO_DISC_DB_DIST_PARAM    = atoi(argv[8]);
    P_i_PROBABILITY_OF_C_t1                     = atoi(argv[9]);
    
    int TIME_INTERVAL_FOR_STATUS = TOTAL_RUN_TIME*20/100;
    struct msqid_ds sys_buffer;
    
    
    alarm(TOTAL_RUN_TIME);
    if (signal(SIGALRM,sighandler)==SIG_ERR){
        fprintf (stderr, "SIGALRM ERROR");
        exit(1);
    }
    if (signal(SIGINT,sighandler)==SIG_ERR){
        fprintf (stderr, "SIGINT ERROR");
        exit(1);
    }
        
    // Creating shared memory segment 
    if((shm_id = shmget(shm_key, sizeof(struct shared_mem_struct), shm_flag))<0) {   // if shmget fails
        fprintf (stderr, "shmget error\n");
        exit(1);
    }    
    // Attaching the shared memory to data space 
    if((shared_mem_temp_ptr = shmat(shm_id, (void *)0, 0)) == (void *)-1){   // if shmat fails
        fprintf (stderr, "shmat error\n");
        exit(1);
    }    
    // Typecast the shared memory block to our structure 
    shared_mem = (struct shared_mem_struct *) shared_mem_temp_ptr;

    // initialize shared memory variables
    shared_mem->produced_request   = 0;
    shared_mem->processed_request  = 0;
    shared_mem->producer_block     = 0;
    shared_mem->consumer_idle      = 0;
    shared_mem->total_block_t      = 0;
    shared_mem->buffer_index_write = 0;
    shared_mem->buffer_index_read  = 0;
    shared_mem->buffer_msg_count   = 0;
    
    
    // BINARY SEMAPHORE
    // shared_mem->sem_key   = 2222;
    // shared_mem->sem_flag  = IPC_CREAT | 0666;
    int sem_key   = 5000;
    int sem_flag  = IPC_CREAT | 0666;
    
    for (int i=0; i<MSGSZ; i++){
        shared_mem->buffer_msg_sizes[i] = 0;
    }
    
    // allocate and initialize semaphore
    // binary_semaphore_allocation (shared_mem->sem_key, shared_mem->sem_flag);
    binary_semaphore_allocation (sem_key, sem_flag);
    binary_semaphore_initialize (shared_mem->sem_id_queue_not_busy);
    
    // initialize the message queue
    msqid = msgget(msqkey, msqflg);
    // msgctl(msqid,IPC_STAT,&shared_mem->sys_buffer);
    // shared_mem->sys_buffer.msg_qbytes = B_BUFFER_SIZE*sizeof(char);
    // shared_mem->sys_buffer.msg_cbytes = 0;
    // shared_mem->sys_buffer.msg_qnum   = 0;
    msgctl(msqid,IPC_STAT,&sys_buffer);
    sys_buffer.msg_qbytes = B_BUFFER_SIZE*sizeof(char);
    sys_buffer.msg_cbytes = 0;
    sys_buffer.msg_qnum   = 0;
    
    // set the owner's user and group ID, the permissions
    // and the size (in number of bytes) of message queue
    // if (msgctl(msqid,IPC_SET,&shared_mem->sys_buffer) == -1) {
    if (msgctl(msqid,IPC_SET,&sys_buffer) == -1) {
      fprintf (stderr, "msgctl: msgctl IPC_SET failed");
      exit (1);
    }
    
    // creating forked processes in these functions
    create_producer_consumer(P_COUNT,C_COUNT);
    // performance_stats(TIME_INTERVAL_FOR_STATUS);
    
    // keeping time for forking separate 
    gettimeofday(&execution_start_time,NULL);
    while(quit == 0){}    
    // waitpid(-1, NULL, 0);
    waitpid(-getpid(), NULL, 0);
    gettimeofday(&execution_end_time,NULL);   
    sleep(1);
    timersub(&execution_end_time,
             &execution_start_time, 
             &total_execution_time);
             
    // removing the message queue specified by msqid
    msgctl(msqid,IPC_RMID,0);
    
    // destroying semaphore
    binary_semaphore_deallocate (shared_mem->sem_id_queue_not_busy);
    
    // accounting
    total_execution_t           = (double)total_execution_time.tv_sec + (double)total_execution_time.tv_usec/1000000.0;
    average_block_t             = shared_mem->total_block_t/(double)P_COUNT;
    average_percentage_block_t  = average_block_t/total_execution_t*100.0;
    percentage_req_processed    = (double)shared_mem->processed_request/(double)shared_mem->produced_request*100;
    percentage_req_blocked      = (double)shared_mem->producer_block/(double)shared_mem->produced_request*100;
    
    fprintf (stderr, "\n\n");
    fprintf (stderr, "Total Execution Time              :\t %.2lf seconds\n", total_execution_t);
    fprintf (stderr, "Average Blocking Time             :\t %.2lf seconds\n", average_block_t);
    fprintf (stderr, "Time Producers Blocked            :\t %.2lf %%\n", average_percentage_block_t);
    fprintf (stderr, "Request Produced                  :\t %lu \n",shared_mem->produced_request);
    fprintf (stderr, "Requests Satisfied                :\t %lu \n",shared_mem->processed_request);
    fprintf (stderr, "Requests Satisfied                :\t %.2lf %%\n", percentage_req_processed);
    fprintf (stderr, "Requests Blocked                  :\t %lu \n",shared_mem->producer_block);
    fprintf (stderr, "Requests Blocked                  :\t %.2lf %%\n", percentage_req_blocked);
    
    // fprintf (stderr, "\n\n");
    // fprintf (stderr, "data_collecttion:\n");
    // fprintf (stderr, "T\t %d\t", TOTAL_RUN_TIME);
    // fprintf (stderr, "B\t %d\t", B_BUFFER_SIZE);
    // fprintf (stderr, "P\t %d\t", P_COUNT);
    // fprintf (stderr, "C\t %d\t", C_COUNT);
    // fprintf (stderr, "Pt\t %0.2f\t", P_t_PRODUCER_DELAY_DIST_PARAM);
    // fprintf (stderr, "Rs\t %0.2f\t", R_s_REQUEST_SIZE_DIST_PARAM);
    // fprintf (stderr, "Ct1\t %0.2f\t", C_t1_CONSUMED_WITH_IO_ONLY_DIST_PARAM);
    // fprintf (stderr, "Ct2\t %0.2f\t", C_t2_CONSUMED_WITH_IO_DISC_DB_DIST_PARAM);
    // fprintf (stderr, "Pi\t %0.2f\t", P_i_PROBABILITY_OF_C_t1);
    // fprintf (stderr, "total_execution_time\t %lf\t", total_execution_t);
    // fprintf (stderr, "Time Producers Blocked %%\t %lf\t", average_percentage_block_t);
    // fprintf (stderr, "Requests Satisfied %%\t %lf\t", percentage_req_processed);
    // fprintf (stderr, "Requests Blocked %%\t %f\n", percentage_req_blocked);
    // fprintf (stderr, "produced_request\t %lu\t", shared_mem->produced_request);
    // fprintf (stderr, "processed_request\t %lu\t", shared_mem->processed_request);
    // fprintf (stderr, "producer_block\t %lu\t", shared_mem->producer_block);
    
    
    // deatach the shared memory 
    if(shmdt(shared_mem) == -1){
        fprintf (stderr, "main: shmdt: detaching shared memory falied\n");
        exit(1);
    }
    // Remove the deatached shared memory 
    if(shmctl(shm_id, IPC_RMID, 0) == -1){
        fprintf (stderr, "main: shmctl: removing shared memory failed\n");
        exit(1);
    }
    
    fprintf (stderr, "THE END \n");
    return 0;
}



int create_producer_consumer(int P_n, int C_n){
    if(P_n<=0 && C_n<=0){
        fprintf (stderr, "All processes created \n");
        return 0;
    }
    pid_t producer_pid;
    pid_t consumer_pid;
   
    if(P_n>0){
        producer_pid= fork();
        if (producer_pid==0){
            produce();
            exit(0);
        }
        else if (producer_pid<0){
            fprintf (stderr, "error in producer \n");
            exit(1);
        }
    }
        
    if(C_n>0){
        consumer_pid= fork();
        if (consumer_pid==0){
            // fprintf (stderr, "Trace : Consumer no. %d PID = %d, GRP_PID = %d\n", C_n, (int)getpid(), getpgrp());
            consume();
            exit(0);
        }
        else if (consumer_pid<0){
            fprintf (stderr, "error in consumer \n");
            exit(1);
        }
    }
    
    create_producer_consumer(P_n-1,C_n-1);
}



void produce(){
    alarm(TOTAL_RUN_TIME);
    // fprintf (stderr, "PRODUCER START\tPID = %d \n", (int)getpid());
    message_buf sending_buf;
    struct timeval produced_i_blocked_start_time;
    struct timeval produced_i_blocked_end_time;
    struct timeval produced_i_blocked_total_time;
    struct timeval produced_i_blocked_temp_time;
    produced_i_blocked_total_time.tv_sec=0;
    produced_i_blocked_total_time.tv_usec=0;
    int blocked=0;
    int is_blocked_time=0;
    
    // Attaching the shared memory to data space 
    if((shared_mem_temp_ptr = shmat(shm_id, (void *)0, 0)) == (void *)-1) {   // if shmat fails
        fprintf (stderr, "Trace: producer: %d shmat FAILD \n", (int)getpid());
        exit(1);
    }
    // Typecast the shared memory block to our structure 
    shared_mem = (struct shared_mem_struct *) shared_mem_temp_ptr;
    // fprintf (stderr, "Trace: producer: %d shared_mem attached \n", (int)getpid());
    
    
    while (quit == 0){
        binary_semaphore_wait(shared_mem->sem_id_queue_not_busy);
        
        int msg_size = get_msg_size(R_s_REQUEST_SIZE_DIST_PARAM);
        shared_mem->produced_request++;
        // msgctl(msqid,IPC_STAT,&shared_mem->sys_buffer); 
        
        // producer is blocked when there is not enough bytes for the current message
        // if((int)shared_mem->sys_buffer.msg_cbytes + (int)strlen(sending_buf.mtext) > (int)B_BUFFER_SIZE){
        if (shared_mem->buffer_msg_count + msg_size  < B_BUFFER_SIZE){
            // for caculateing time 
            if (blocked == 1){
                is_blocked_time = 1;
                gettimeofday(&produced_i_blocked_end_time,NULL);
                blocked = 0;
            }
            
            // creating the message to send
            sending_buf.mtype  = 1;
            char* msg_str = malloc((msg_size)*sizeof(char));
            memset(msg_str, 'x', msg_size);
            strcpy (sending_buf.mtext, msg_str);
            free (msg_str);
            
            // simulate delay
            useconds_t t =get_producer_delay(P_t_PRODUCER_DELAY_DIST_PARAM);
            usleep(t);
            msgsnd(msqid, &sending_buf, msg_size, 0);
            // fprintf (stderr, "PRODUCER: %d Message Sent \t msgsz= %d\t%s\n", (int)getpid(), msg_size, sending_buf.mtext);
            
            shared_mem->buffer_msg_sizes[shared_mem->buffer_index_write] = msg_size;
            shared_mem->buffer_index_write = (shared_mem->buffer_index_write + msg_size) % B_BUFFER_SIZE;
            shared_mem->buffer_msg_count += msg_size;
            
            // if (msgsnd(msqid, &sending_buf, msg_size, 0) < 0){
                // fprintf (stderr, "PRODUCER: %d msgsnd: msgsnd failed", (int)getpid());
                // exit (1);	  
            // }
        } else {
            gettimeofday(&produced_i_blocked_start_time,NULL);
            blocked = 1;
            shared_mem->producer_block++;
        }
        
        if (is_blocked_time == 1){            
            is_blocked_time = 0;
            timersub(&produced_i_blocked_end_time,
                     &produced_i_blocked_start_time, 
                     &produced_i_blocked_temp_time);
            
            timeradd(&produced_i_blocked_total_time,
                     &produced_i_blocked_temp_time, 
                     &produced_i_blocked_total_time);
            
            shared_mem->total_block_t += (double)produced_i_blocked_temp_time.tv_sec + 
                                         (double)produced_i_blocked_temp_time.tv_usec/1000000.0;

        }
                 
        binary_semaphore_post(shared_mem->sem_id_queue_not_busy);
    }
    
    fprintf (stderr, "Producer: %d total blocked time =\t %lf \t seconds\n", (int)getpid(), 
                                (double)produced_i_blocked_total_time.tv_sec + 
                                (double)produced_i_blocked_total_time.tv_usec/1000000.0);

    // deatach the shared memory 
    if(shmdt(shared_mem) == -1){
        fprintf (stderr, "producer : %d \t shmdt: detaching shared memory falied\n", (int)getpid());
        exit(1);
    }
    
    // fprintf (stderr, "PRODUCER END\tPID = %d \n", (int)getpid());
    exit(0);
}



void consume(){
    alarm(TOTAL_RUN_TIME);
    // fprintf (stderr, "CONSUMER START\tPID = %d \n", (int)getpid());
    message_buf receiving_buf;
    
    
    // Attaching the shared memory to data space 
    if((shared_mem_temp_ptr = shmat(shm_id, (void *)0, 0)) == (void *)-1){   // if shmat fails
        fprintf (stderr, "Trace: consumer: %d shmat FAILD \n", (int)getpid());
        exit(1);
    }
    // Typecast the shared memory block to our structure 
    shared_mem = (struct shared_mem_struct *) shared_mem_temp_ptr;
    
    
    while(quit == 0){
        binary_semaphore_wait(shared_mem->sem_id_queue_not_busy);
        
        // msgctl(msqid,IPC_STAT,&shared_mem->sys_buffer);
        
        if((int)shared_mem->buffer_msg_count > 0) {
            int msg_size = shared_mem->buffer_msg_sizes[shared_mem->buffer_index_read];
            // if (msgrcv(msqid, &receiving_buf, B_BUFFER_SIZE, 1, 0) < 0){
                // fprintf (stderr, "msgrcv: msgrcv failed");
                // exit (1);	  
            // }
            msgrcv(msqid, &receiving_buf, msg_size, 1, 0);
            // fprintf (stderr, "CONSUMER: %d Message received msgsz= %d\t%s by  \n", (int)getpid(), msg_size, receiving_buf.mtext);
            
            // simulating the time to consume data
            useconds_t t = get_consumer_delay(P_i_PROBABILITY_OF_C_t1);
            usleep(t);
            
            shared_mem->buffer_msg_sizes[shared_mem->buffer_index_read] = 0;
            shared_mem->buffer_index_read = (shared_mem->buffer_index_read + msg_size) % B_BUFFER_SIZE;
            shared_mem->buffer_msg_count -= msg_size;
            
            shared_mem->processed_request++;
        }
        else {
            shared_mem->consumer_idle++;
        }
          
        binary_semaphore_post(shared_mem->sem_id_queue_not_busy);
    }
    
    
    // deatach the shared memory 
    if(shmdt(shared_mem) == -1){
        fprintf (stderr, "Trace: consumer : %d \t shmdt: detaching shared memory falied\n", (int)getpid());
        exit(1);
    }
        
    // fprintf (stderr, "CONSUMER END\tPID = %d \n", (int)getpid());
    exit(0);
}



void performance_stats (int status_interval_t){
    alarm(TOTAL_RUN_TIME);
    pid_t stats_pid;
    stats_pid = fork();
    if (stats_pid==0){
        struct timeval start_time;
        struct timeval current_time;
        struct timeval elapsed_time;
        
        gettimeofday(&start_time,NULL);
        
        // Attaching the shared memory to data space 
        if((shared_mem_temp_ptr = shmat(shm_id, (void *)0, 0)) == (void *)-1) {   // if shmat fails
            fprintf (stderr, "Trace: producer: %d shmat FAILD \n", (int)getpid());
            exit(1);
        }
        // Typecast the shared memory block to our structure 
        shared_mem = (struct shared_mem_struct *) shared_mem_temp_ptr;
        
        
        while (quit==0) {
            sleep(status_interval_t);
            gettimeofday(&current_time,NULL);
            timersub(&current_time, &start_time, &elapsed_time);
                    
            fprintf (stderr, "Time: %lf seconds\tproduced: %lu\tprocessed: %lu\tblocked: %lu\n", 
                                        ((double)elapsed_time.tv_sec + (double)elapsed_time.tv_usec/1000000.0),
                                        shared_mem->produced_request,
                                        shared_mem->processed_request,
                                        shared_mem->producer_block);
        }
        
        
        // deatach the shared memory 
        if(shmdt(shared_mem) == -1){
            fprintf (stderr, "Trace: consumer : %d \t shmdt: detaching shared memory falied\n", (int)getpid());
            exit(1);
        }
        
        exit(0);
    }
}



useconds_t get_producer_delay(double P_t_param){
    return (useconds_t)(100000.0*normal_distribution (P_t_param));
}


useconds_t get_consumer_delay(double probability){
    double x;
    srand(time(NULL)*getpid());
    x=(double)rand()/(double)(RAND_MAX);
    if(x<=probability) {
        return (useconds_t)(100000.0*normal_distribution(C_t1_CONSUMED_WITH_IO_ONLY_DIST_PARAM));
    } else {
        return (useconds_t)(100000.0*normal_distribution(C_t2_CONSUMED_WITH_IO_DISC_DB_DIST_PARAM));
    }
}


int get_msg_size(int req_size){
    int c;
    int r;
    srand(time(NULL)*getpid()+(int)req_size);
    c=rand();
    if(c!=0){
        r = (int)(c % B_BUFFER_SIZE);
        // if (B_BUFFER_SIZE < MSGSZ)
            // r = (int)(c % B_BUFFER_SIZE);
        // else 
            // r = (int)(c % MSGSZ);
        
        if (r == 0){
            get_msg_size(req_size);
        } else {                 
            return abs(r);
        }
    } else {
        get_msg_size(req_size);
    }
}


// assuming mu = 0 for this normal distribution function
// and sigma is the param that have been entered in the cmd line
double normal_distribution (double sigma) {
    double x,y;
    srand(time(NULL)*getpid());
    x = (double)rand()/(double)(RAND_MAX)*sigma;
    if(x!=0) {
        y=(exp(-(x*x)/(2*sigma*sigma)))/(sqrt(2*3.1416)*sigma);
        return y;
    } else {
        normal_distribution(sigma);
    }
}


void sighandler(int signum) {
    if (signum==SIGINT){
        fprintf (stderr, "SIGNAL = SIGINT \n");
        quit=1;
    }
    if (signum==SIGALRM){
        // fprintf (stderr, "SIGNAL = SIGALRM after %d sec\n", TOTAL_RUN_TIME);
        quit=1;
    }
}


/* Obtain a binary semaphore’s ID, allocating if necessary. */
int binary_semaphore_allocation (key_t key, int sem_flags){
    return semget (key, 1, sem_flags);
}


/* Deallocate a binary semaphore. All users must have finished their
use. Returns -1 on failure. */
int binary_semaphore_deallocate (int semid){
    union semun ignored_argument;
    return semctl (semid, 1, IPC_RMID, ignored_argument);
}


/* Initialize a binary semaphore with a value of 1. */
int binary_semaphore_initialize (int semid){
    union semun argument;
    unsigned short values[1];
    values[0] = 1;
    argument.array = values;
    return semctl (semid, 0, SETALL, argument);
}


/* Wait on a binary semaphore. 
Block until the semaphore value is positive, then decrement it by 1. */
int binary_semaphore_wait (int semid){
    struct sembuf operations[1]; /* Use the first (and only) semaphore. */
    operations[0].sem_num = 0; /* Decrement by 1. */
    operations[0].sem_op = -1; /* Permit undo’ing. */
    operations[0].sem_flg = SEM_UNDO;
    return semop (semid, operations, 1);
        
    // struct sembuf sops[1];
    // sops[0].sem_num = 0; /* We only use one track */
    // sops[0].sem_op = 0; /* wait for semaphore flag to become zero */
    // sops[0].sem_flg = SEM_UNDO; /* take off semaphore asynchronous  */
    // sops[1].sem_num = 0;
    // sops[1].sem_op = 1; /* increment semaphore -- take control of track */
    // sops[1].sem_flg = SEM_UNDO | IPC_NOWAIT; /* take off semaphore */
    // return semop (semid, sops, 1);
}


/* Post to a binary semaphore: increment its value by 1.
This returns immediately. */
int binary_semaphore_post (int semid){
    struct sembuf operations[1];
    operations[0].sem_num = 0; /* Use the first (and only) semaphore. */
    operations[0].sem_op = 1; /* Increment by 1. */
    operations[0].sem_flg = SEM_UNDO; /* Permit undo’ing. */
    return semop (semid, operations, 1);    
}


