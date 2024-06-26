#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>  // perror()
#include <stdlib.h> // exit()
#include <fcntl.h>  // open()
#include <unistd.h> // dup2() and execl, execlp, execv, execvp
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <sys/ipc.h> 
#include <errno.h> 
#include <sys/sem.h> 
#include <sys/stat.h> 
#include <pthread.h>
#include <sys/sem.h>
#include <gpiod.h>



#define SEM_MODE 0666 /* rw(owner)-rw(group)-rw(other) permission */ 
#define SEM_KEY_STACK   1122334455 
#define SEM_KEY_ARM     1112223334 
#define SEM_KEY_COUNT    1112223333
#define SEM_MULTI_CLIENTS 1112223344
#define SEM_TEMP         1112223444
#define MAX_client 5
#define MAX_stack_size 4
#define END_CMD 10

// for robot arm control 
typedef enum Command{
  NOP       = 0,
  //
  stack1    = 1,
  stack2    = 2,
  stack3    = 3,
  stack4    = 4,
  //
  readypos  = 5,
  pos1      = 6,
  pos2      = 7,
  pos3      = 8,
  //
  ESTOP     = 9,
  hold      = 10,
  resume    = 11,
  svon      = 12,
  svoff     = 13,
  reset     = 14,
  reserved  = 15
} Command;
typedef struct Robot{
     struct gpiod_chip *chip;
     struct gpiod_line_request_config config;
     struct gpiod_line_bulk lines;
     struct gpiod_line_bulk lines_read;
     struct gpiod_line_request_config config_read;
     unsigned int gpio_out[5];
     int status;  // 0: idle, 1: active , 2: busy
}Robot;
int RobotCommand(Robot* rb, Command cmd);
void initRobot(Robot* rb, int* gpio_assign);
void initRead(Robot* rb, int* gpio_assign);
int readStatus(Robot* rb);

Robot rb[2];



//gloabal variable for different threads or functions
int sockfd;
int current_stack = 0; 
int sem_stack,sem_arm,sem_counting,sem_multi_clients, sem_temp;
double robot_stage1[2]; //represent robot arm control data
double robot_stage2[2]; //represent robot arm control data
int robot_active[2]={0,0}; //record robot arms are avaliable or not //0:avaliable ,1:active

/* P () - returns 0 if OK; -1 if there was a problem */ 
int P (int s)  
{ 
 struct sembuf sop; /* the operation parameters */ 
 sop.sem_num =  0; /* access the 1st (and only) sem in the array */ 
 sop.sem_op  = -1;    /* wait..*/ 
 sop.sem_flg =  0;    /* no special options needed */ 
 
 if (semop (s, &sop, 1) < 0) {  
  fprintf(stderr,"P(): semop failed: %s\n",strerror(errno)); 
  return -1; 
 } else { 
  return 0; 
 } 
} 

/* V() - returns 0 if OK; -1 if there was a problem */ 
int V(int s) 
{ 
 struct sembuf sop; /* the operation parameters */ 
 sop.sem_num =  0; /* the 1st (and only) sem in the array */ 
 sop.sem_op  =  1; /* signal */ 
 sop.sem_flg =  0; /* no special options needed */ 
 
 if (semop(s, &sop, 1) < 0) {  
  fprintf(stderr,"V(): semop failed: %s\n",strerror(errno)); 
  return -1; 
 } else { 
  return 0; 
 } 
}

//ctrl+c instruction
void sigint_handler(int signum){
    //close semaphore
    if (semctl (sem_stack, 0, IPC_RMID, 0) < 0) 
    { 
        fprintf (stderr, "unable to remove sem %d\n", SEM_KEY_STACK); 
        exit(1); 
    } 
    if (semctl (sem_arm, 0, IPC_RMID, 0) < 0) 
    { 
        fprintf (stderr, "unable to remove sem %d\n", SEM_KEY_ARM); 
        exit(1); 
    } 
    if (semctl (sem_counting, 0, IPC_RMID, 0) < 0) 
    { 
        fprintf (stderr, "unable to remove sem %d\n", SEM_KEY_COUNT); 
        exit(1); 
    } 
    if (semctl (sem_multi_clients, 1, IPC_RMID, 0) < 0) 
    { 
        fprintf (stderr, "unable to remove sem %d\n", SEM_MULTI_CLIENTS); 
        exit(1); 
    }
    if (semctl (sem_temp, 1, IPC_RMID, 0) < 0) 
    { 
        fprintf (stderr, "unable to remove sem %d\n", SEM_MULTI_CLIENTS); 
        exit(1); 
    }
    //close scoketfd
    close(sockfd);
    //program end
    exit(signum);
}

void *pick_place(void *arg) {
    // Robot* rb = (Robot*)arg; // Uncomment and initialize the Robot pointer appropriately
    printf("Pick place: Started, ID = %lu.\n", (unsigned long)pthread_self());
    int arm_id = 0; 

    // Check if a robot arm is available
    P(sem_counting);
    P(sem_arm);
    
    if (rb[0].status == 0) {
        rb[0].status = 1; 
        arm_id = 0; // Robot arm 1
    } else {
        rb[1].status = 1;
        arm_id = 1; // Robot arm 2
    }
    V(sem_arm);
    
    // Move to the ready position
    printf("arm_id %d is commanded to [READYPOS].\n", arm_id);

    // Only one robot arm can be commanded at a time
    P(sem_temp);
    RobotCommand(&rb[arm_id], readypos); 
    V(sem_temp);
    //usleep(250000);
    
    // Wait while the arm is busy (make sure the arm is at readypos)
    P(sem_temp);
    int prev_status = -1; // Initialize previous status with an invalid value
    while (readStatus(&rb[arm_id])) {
        if (prev_status != 1) { // Only print when status changes
            //printf("arm_id %d is moving to [READYPOS].\n", arm_id);
            prev_status = 1;
        }
        // prevent busy waiting
        usleep(250000);
    }
    if (prev_status != 0) {
        printf("arm_id %d is at [READYPOS].\n", arm_id); // Print when arm becomes ready
    }
    V(sem_temp);

    // Control the robot arm (with current_stack), checking the current stack
    P(sem_stack);
    RobotCommand(&rb[arm_id], (Command)(current_stack + 1)); 
    
    //usleep(250000);
    printf("arm_id %d is commanded to [STACK %d].\n", arm_id, current_stack + 1);
    current_stack += 1;

    // Wait while the arm is busy (make sure the arm completes the current stack)
    prev_status = -1; // Reinitialize previous status
    while (readStatus(&rb[arm_id])) {
        if (prev_status != 1) {
            //printf("arm_id %d is [STACKING].\n", arm_id);
            prev_status = 1;
        }
        // prevent busy waiting
        usleep(250000);
    }
    if (prev_status != 0) {
        printf("arm_id %d completed [STACKING].\n", arm_id); // Print when stacking is done
    }
    V(sem_stack);
    
    // Release the robot arm semaphore
    P(sem_arm);
    rb[arm_id].status = 0; // Robot arm finished
    V(sem_arm);
    V(sem_counting);

    pthread_exit(NULL);
}




//a client routine
void *command_reciever(void *fd){
    //Init
    pthread_t thread_id = pthread_self();
    int *tmp=(int*)fd;
    int forClientSockfd;
    forClientSockfd=*tmp;
    
    free(fd);

    //create threads to control robot arm
    pthread_t threads[MAX_stack_size];
    int t=0;
    int rc;

    //communication
    int client_command;
    char inputBuffer[256];
    char outputBuffer[256];
    while (1)
    {

        if (recv(forClientSockfd,inputBuffer,sizeof(inputBuffer),0) != -1){
            
            client_command=atoi(inputBuffer);
            P(sem_multi_clients); // only one client can send command at a time
            // printf("Client fd : %d\n",forClientSockfd);
            #ifdef DEBUG
                goto test;
            #endif
            if(client_command==END_CMD){
                //ending client_command
                break;
            }
            P(sem_stack);
            if (current_stack >= MAX_stack_size){
                //stack is full
                current_stack = 0;
            }
            V(sem_stack);

            //client_command decoding
            switch (client_command)
            {
            case 0:
                //do client_command 0
                printf("do client_command 0\n");
                break;
            case 1:
                for(int i = 0;i < 1;i++){
                    rc = pthread_create(&threads[i], NULL, pick_place, (void *)rb);
                    if (rc){
                        printf("ERROR; pthread_create() returns %d\n", rc);
                        exit(-1);
                        break;
                    }
                    printf("do client_command 1\n");
                }
                // make sure all threads are finished
                for(int i = 0;i < 1;i++){
                    pthread_join(threads[i], NULL);
                }
                V(sem_multi_clients); // release semaphore
                current_stack = 0;
                break;
            case 2:
                for(int i = 0;i < 2;i++){
                    rc = pthread_create(&threads[i], NULL, pick_place, (void *)rb);
                    if (rc){
                        printf("ERROR; pthread_create() returns %d\n", rc);
                        exit(-1);
                        break;
                    }
                    printf("do client_command 2\n");
                }
                // make sure all threads are finished
                for(int i = 0;i < 2;i++){
                    pthread_join(threads[i], NULL);
                }
                V(sem_multi_clients); // release semaphore
                current_stack = 0;
                break;

            case 3:
                for(int i = 0;i < 3;i++){
                    rc = pthread_create(&threads[i], NULL, pick_place, (void *)rb);
                    if (rc){
                        printf("ERROR; pthread_create() returns %d\n", rc);
                        exit(-1);
                        break;
                    }
                    printf("do client_command 3\n");
                }
                // make sure all threads are finished
                for(int i = 0;i < 3;i++){
                    pthread_join(threads[i], NULL);
                }
                V(sem_multi_clients); // release semaphore
                current_stack = 0;
                break;
                //do client_command 1
                //create pick_place thread to control arm
            case 4:
                for(int i = 0;i < 4;i++){
                    rc = pthread_create(&threads[i], NULL, pick_place, (void *)rb);
                    if (rc){
                        printf("ERROR; pthread_create() returns %d\n", rc);
                        exit(-1);
                        break;
                    }
                    printf("do client_command 4\n");
                }
                // make sure all threads are finished
                for(int i = 0;i < 4;i++){
                    pthread_join(threads[i], NULL);
                }
                V(sem_multi_clients); // release semaphore
                current_stack = 0;
                break; 

            default:
                break;
            }
            
        }
    }
    #ifdef DEBUG
        test:
        V(sem_multi_clients);
    #endif
    close(forClientSockfd);
    pthread_exit(NULL);
}




int main(int argc, char *argv[]) {
    signal(SIGINT,sigint_handler);
    unsigned int rb1_gpio_assign[5] = {23, 4, 17, 27, 22};
    initRobot(rb, rb1_gpio_assign);
    unsigned int rb1_gpio_read[1] = {5};
    initRead(rb, rb1_gpio_read);

    unsigned int rb2_gpio_assign[5] = {24, 10, 9, 11, 0}; // done
    initRobot(rb+1, rb2_gpio_assign);
    unsigned int rb2_gpio_read[1] = {6};
    initRead(rb+1, rb2_gpio_read);

     //semaphore (two binary and one counting semaphore)
    sem_stack = semget(SEM_KEY_STACK, 1,IPC_CREAT | IPC_EXCL| SEM_MODE); 
    if (sem_stack < 0) 
    { 
        fprintf(stderr, "Sem %d creation failed: %s\n", SEM_KEY_STACK,  strerror(errno)); 
        exit(-1); 
    } 
    sem_arm = semget(SEM_KEY_ARM, 1,IPC_CREAT | IPC_EXCL| SEM_MODE); 
    if (sem_arm < 0) 
    { 
        fprintf(stderr, "Sem %d creation failed: %s\n", SEM_KEY_ARM,  strerror(errno)); 
        exit(-1); 
    } 
    sem_counting = semget(SEM_KEY_COUNT, 2,IPC_CREAT | IPC_EXCL| SEM_MODE); 
    if (sem_arm < 0) 
    { 
        fprintf(stderr, "Sem %d creation failed: %s\n", SEM_KEY_COUNT,  strerror(errno)); 
        exit(-1); 
    } 
    sem_multi_clients = semget(SEM_MULTI_CLIENTS, 1,IPC_CREAT | IPC_EXCL| SEM_MODE);
    if (sem_multi_clients < 0) 
    { 
        fprintf(stderr, "Sem %d creation failed: %s\n", SEM_MULTI_CLIENTS,  strerror(errno)); 
        exit(-1); 
    }
    sem_temp = semget(SEM_TEMP, 1,IPC_CREAT | IPC_EXCL| SEM_MODE);
    if (sem_multi_clients < 0) 
    { 
        fprintf(stderr, "Sem %d creation failed: %s\n", SEM_MULTI_CLIENTS,  strerror(errno)); 
        exit(-1); 
    }
    /* initial semaphore value to 1 (binary semaphore) */ 
    if ( semctl(sem_stack, 0, SETVAL, 1) < 0 ) 
    { 
        fprintf(stderr, "Unable to initialize Sem: %s\n", strerror(errno)); 
        exit(0); 
    }
    if ( semctl(sem_arm, 0, SETVAL, 1) < 0 ) 
    { 
        fprintf(stderr, "Unable to initialize Sem: %s\n", strerror(errno)); 
        exit(0); 
    }
    if ( semctl(sem_counting, 0, SETVAL, 2) < 0 ) 
    { 
        fprintf(stderr, "Unable to initialize Sem: %s\n", strerror(errno)); 
        exit(0); 
    }
    if ( semctl(sem_multi_clients, 0, SETVAL, 1) < 0 ) 
    { 
        fprintf(stderr, "Unable to initialize Sem: %s\n", strerror(errno)); 
        exit(0); 
    }
    if ( semctl(sem_temp, 0, SETVAL, 1) < 0 ) 
    { 
        fprintf(stderr, "Unable to initialize Sem: %s\n", strerror(errno)); 
        exit(0); 
    }



    //socket server initialization
    int yes = 1;
    sockfd= socket(PF_INET,SOCK_STREAM,0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in sever;
    memset(&sever, 0, sizeof(sever));
    sever.sin_family = AF_INET;
    sever.sin_addr.s_addr = INADDR_ANY;
    sever.sin_port = htons((u_short)atoi(argv[1]));
    
    bind(sockfd, (struct sockaddr *) &sever, sizeof(sever));
    listen(sockfd,MAX_client);

    struct sockaddr_in clientInfo;
    int addrlen = sizeof(clientInfo);
    int forClientSockfd=-1;
    pthread_t threads[MAX_client];
    int t=0;
    int rc;
    while(1){
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
        if (forClientSockfd!=-1){
            int *clientfd_ptr = (int*)malloc(sizeof(int));
            if (clientfd_ptr == NULL) {
                perror("Failed to allocate memory");
                exit(EXIT_FAILURE);
            }
            *clientfd_ptr = forClientSockfd;    

            rc = pthread_create(&threads[t], NULL, command_reciever, (void *)clientfd_ptr);
            
            t=t%MAX_client;
            t=t+1;
            if (rc){
                printf("ERROR; pthread_create() returns %d\n", rc);
                exit(-1);
            }
            // 
        }
        forClientSockfd=-1;

    }
    

}

int RobotCommand(Robot* rb, Command cmd){
    int ret = 0;
    int err = 0;
    // Command cmd_enum = (Command)cmd;
    switch(cmd){
        case ESTOP:{
            int values[5] = {0, 1, 0, 0, 1};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            break;
        }
        case hold:{
            int values[5] = {0, 1, 0, 1, 0};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);            
            break;
        }
        case resume:{
            int values[5] = {0, 1, 0, 1, 1};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);           
            break;
        }
        case svon:{
            int values[5] = {0, 1, 1, 0, 0};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            printf("Servo ON\n");
            break;
        }
        case svoff:{
            int values[5] = {0, 1, 1, 0, 1};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            printf("Servo OFF\n");            
            break;
        }
        case stack1:{
            int values[5] = {0, 0, 0, 0, 1};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);           
            break;
        }
        case  stack2:{
            int values[5] = {0, 0, 0, 1, 0};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);            
            break;
        }
        case  stack3:{
            int values[5] = {0, 0, 0, 1, 1};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);            
            break;
        }
        case  stack4:{
            int values[5] = {0, 0, 1, 0, 0};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);            
            break;
        }
        case  readypos:{
            int values[5] = {0, 0, 1, 0, 1};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);            
            break;
        }
        case  pos1:{
            int values[5] = {0, 0, 1, 1, 0};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);            
            break;
        }
        case  pos2:{
            int values[5] = {0, 0, 1, 1, 1};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);            
            break;
        }
        case  pos3:{
            int values[5] = {0, 1, 0, 0, 0};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);            
            break;
        }
        case  reset:{
            int values[5] = {0, 1, 1, 1, 0};
            err = gpiod_line_set_value_bulk(&rb->lines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(&rb->lines, values);            
            break;
        }
        case  NOP:{
            ret = -1;
            break;
        }
    }
    if(err){
        perror("gpiod_line_set_value_bulk");
        ret = -1;
    }
    return ret;
}



void initRobot(Robot* rb, int* gpio_assign){
    memcpy(rb->gpio_out, gpio_assign, 5*sizeof(int));
    rb->chip = gpiod_chip_open("/dev/gpiochip4");
    if(!rb->chip){
        perror("gpiod_chip_open");
        goto cleanup;
    }
    int err = gpiod_chip_get_lines(rb->chip, gpio_assign, 5, &rb->lines);
    if(err){
        perror("gpiod_chip_get_lines");
        goto cleanup;
    }

    memset(&rb->config, 0, sizeof(rb->config));
    rb->config.consumer = "robot_command";
    rb->config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
    rb->config.flags = 0;

    int values[5] = {0, 0, 0, 0, 0};
    // get the bulk lines setting default value to 0
    err = gpiod_line_request_bulk(&rb->lines, &rb->config, values);
    if(err)
    {
        perror("gpiod_line_request_bulk");
        goto cleanup;

    }
    
    
    cleanup:
        sleep(0);
}

// send request to gpio for reading
void initRead(Robot* rb, int* gpio_assign){
    // memcpy(rb->gpio_out, gpio_assign, sizeof(int));
    // rb->chip = gpiod_chip_open("/dev/gpiochip4");
    // if(!rb->chip){
    //     perror("gpiod_chip_open");
    //     goto cleanup;
    // }
    // read
    int err = gpiod_chip_get_lines(rb->chip, gpio_assign, 1, &rb->lines_read);
    if(err){
        perror("gpiod_chip_get_lines");
        goto cleanup_2;
    }

    memset(&rb->config_read, 0, sizeof(rb->config_read));
    rb->config_read.consumer = "robot_status";
    rb->config_read.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
    rb->config_read.flags = 0;

    int r_values[1] = {0};
    // get the bulk lines setting default value to 0
    err = gpiod_line_request_bulk(&rb->lines_read, &rb->config_read, r_values);
    if(err)
    {
        perror("gpiod_line_request_bulk");
        goto cleanup_2;

    }
    cleanup_2:
        sleep(0);
} 

// read the value from gpio
int readStatus(Robot* rb){
    int ret = 0;
    int err = 0;
    int values[1] = {0};
    int value = 0;
    // value = gpiod_line_get_value(&rb->lines_read);
    err = gpiod_line_get_value_bulk(&rb->lines_read, values);
    if(err){
        perror("gpiod_line_get_value_bulk");
        ret = -1;
    }
    // return value;
    return values[0];
}

