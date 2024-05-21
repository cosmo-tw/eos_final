#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char *message;
    char buffer[1024] = {0};

    printf("Enter the number of objects to pick: ");
    int num_objects;
    scanf("%d", &num_objects);

    if (num_objects <= 0) {
        printf("Invalid input. Please enter a positive number.\n");
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed\n");
        return -1;
    }

    // Convert the number of objects to a string and send it to the server
    asprintf(&message, "%d", num_objects);
    send(sock, message, strlen(message), 0);
    printf("Number of objects to pick sent: %s\n", message);

    close(sock);
    free(message);
    return 0;
}
