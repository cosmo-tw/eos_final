#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>

#define END_CMD 10
#define DEMO

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

    //socket creation
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket connection
    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    info.sin_addr.s_addr = inet_addr(argv[1]);//ip address
    info.sin_port = htons(atoi(argv[2]));//port
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error");
    }


    int i;
    int command[10]={0,1,2,3,4,5,6,7,8,9};
    char tmp;
    int sel = 0;
    #ifdef DEMO
    if (argc == 4)
        sel = atoi(argv[3]);
    #endif
    char message[256];
    while (1)
    {
        printf("Send a command\n");
        #ifdef DEMO
        if (sel == 0)
        #endif
            scanf("%d",&sel);
        // system("clear");
        if (sel==10){
            break;
        }
        // while ((tmp = getchar()) != '\n') {}
        memset(message,'\0',sizeof(message));
        sprintf(message,"%d",sel);
        send(sockfd,message,sizeof(message),0);
        if(sel==END_CMD){
            break;
        }
        #ifdef DEMO
        sel = 0;
        #endif
    }
    
    close(sockfd);
    return 0;
}