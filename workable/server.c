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
#define SEM_MODE 0666 /* rw(owner)-rw(group)-rw(other) permission */ 
#define SEM_KEY_STACK   1122334455 
#define SEM_KEY_ARM     1112223334 
#define SEM_KEY_COUNT    1112223333
#define MAX_client 5
#define MAX_stack_size 4
#define END_CMD 10

typedef enum Command{
  NOP       = 0,
  //
  stack1    = 1,
  stack2    = 2,
  stack3    = 3,
  stack4    = 4,
  //
  pos0      = 5,
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

//gloabal variable for different threads or functions
int sockfd;
int current_stack=0;
int sem_stack,sem_arm,sem_counting;
double robot_stage1[2];//represent robot arm control data
double robot_stage2[2];//represent robot arm control data
int robot_active[2]={0,0};//record robot arms are avaliable or not //0:avaliable ,1:active

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
    //close scoketfd
    close(sockfd);
    //program end
    exit(signum);
}

//A robot arm control routine
void *pick_place(void *arg) {
    printf("pick_place: Started, ID=%d\n",(int)pthread_self());
    int arm_id;
    //check robot arm is avaliable
    P(sem_counting);
    P(sem_arm);
    if (robot_active[0]==0){
        robot_active[0]==1;
        arm_id=0;
    }
    else{
        robot_active[1]==1;
        arm_id=1;
    }
    V(sem_arm);

    //control robot arm (stage 1)
    //control_command(robot_stage1[arm_id]);

    //control robot arm (stage 2) needs to check current stack
    P(sem_stack);
    //control_command(robot_stage2[arm_id],current_stack);
    current_stack+=1;
    V(sem_stack);
    
    //release robot arm
    P(sem_arm);
    robot_active[arm_id]==0;
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
            if(client_command==END_CMD){
                //ending client_command
                break;
            }
            //client_command decoding
            switch (client_command)
            {
            case 0:
                //do client_command 0
                printf("do client_command 0\n");
                break;
            case 3:
                for(int i = 0;i < 3;i++){
                    rc = pthread_create(&threads[i], NULL, pick_place, NULL);
                    if (rc){
                        printf("ERROR; pthread_create() returns %d\n", rc);
                        exit(-1);
                        break;
                    }
                    printf("do client_command 3\n");
                }
                break;
                //do client_command 1
                //create pick_place thread to control arm
                

            default:
                break;
            }
            
        }
    }
    

    close(forClientSockfd);
    pthread_exit(NULL);
}




int main(int argc, char *argv[]) {
    signal(SIGINT,sigint_handler);

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

        }
        forClientSockfd=-1;

    }


}