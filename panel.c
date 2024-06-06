#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sys/ipc.h> 

#define END_CMD 10
#define DEMO

int sockfd;
void sigint_handler(int signum){
    //close scoketfd
    close(sockfd);
    //program end
    exit(signum);
}
//reference from https://stackoverflow.com/questions/7469139/what-is-the-equivalent-to-getch-getche-in-linux
char getch(void)
{
    char buf = 0;
    struct termios old = {0};
    fflush(stdout);
    if(tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if(tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if(read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if(tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return buf;
 }

int main(int argc , char *argv[])
{
    signal(SIGINT,sigint_handler);
    //socket creation
    int yes = 1;
    sockfd= socket(PF_INET,SOCK_STREAM,0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in sever;
    memset(&sever, 0, sizeof(sever));
    sever.sin_family = AF_INET;
    sever.sin_addr.s_addr = INADDR_ANY;
    sever.sin_port = htons((u_short)6666);
    
    bind(sockfd, (struct sockaddr *) &sever, sizeof(sever));
    listen(sockfd,1);

    struct sockaddr_in clientInfo;
    int addrlen = sizeof(clientInfo);
    int forClientSockfd=-1;
    int pid;
    int done=1;
    char inputBuffer[256];
    while(done){
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
        if (forClientSockfd!=-1){
            while (1)
            {
                if (recv(forClientSockfd,inputBuffer,sizeof(inputBuffer),0) != -1){
                    pid=atoi(inputBuffer);
                    break;
                }
            }
           
           done=0;
        }
    }
    close(sockfd);

    //panel command
    int sel;
    char tmp;
    //send hold or resume
    while (1)
    {
        printf("hold or resume?\n1: hold\n2: resume\n3: close\n");
        scanf("%d",&sel);
        // system("clear");
        if (sel==3){
            break;
        }
        else if(sel==1){
            kill(pid, SIGUSR1);
        }
        else if(sel==2){
            kill(pid, SIGUSR2);
        }
        while ((tmp = getchar()) != '\n') {}
    }

    return 0;
}