// This is the server code for the final project of the course "Embedded Operating Systems".
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

// Global variables
int number_of_objects = 0;
int objects_to_pick = 0;

// Semaphore declaration
sem_t sem;

// Thread variables
pthread_t thread1, thread2;
pthread_mutex_t lock;

// Function to simulate the pick and place operation
void pick_and_place(int current_value) {
    printf("Picking and placing an object based on current_value = %d\n", current_value);

    sleep(1); // Simulating the pick and place task
}

void pick_ready_1() {
    printf("Robot Arm one is ready to pick an object\n");
}



// Function for the first robot arm
void* robot_arm_1(void* arg) {
    while (1) {
        pick_ready_1();
        sem_wait(&sem); // Acquire semaphore
        if (number_of_objects < objects_to_pick) {
            int current_value = number_of_objects;
            printf("Robot Arm 1: Reading number_of_objects = %d\n", current_value);
            pick_and_place(current_value);
            number_of_objects++;
            printf("Robot Arm 1: Updated number_of_objects to %d\n", number_of_objects);
        }
        sem_post(&sem); // Release semaphore
        sleep(1); // Adding delay to avoid busy waiting
    }
    return NULL;
}

// Function for the second robot arm
void* robot_arm_2(void* arg) {
    while (1) {
        pick_ready_2();
        sem_wait(&sem); // Acquire semaphore
        if (number_of_objects < objects_to_pick) {
            int current_value = number_of_objects;
            printf("Robot Arm 2: Reading number_of_objects = %d\n", current_value);
            pick_and_place(current_value);
            number_of_objects++;
            printf("Robot Arm 2: Updated number_of_objects to %d\n", number_of_objects);
        }
        sem_post(&sem); // Release semaphore
        sleep(1); // Adding delay to avoid busy waiting
    }
    return NULL;
}

// Function to handle each client connection
void* handle_client(void* socket_desc) {
    int new_socket = *(int*)socket_desc;
    char buffer[1024] = {0};
    read(new_socket, buffer, 1024);
    int num_objects = atoi(buffer);
    printf("Received command to pick %d objects\n", num_objects);

    pthread_mutex_lock(&lock);
    objects_to_pick = num_objects;
    number_of_objects = 0; // Reset the number of objects picked
    pthread_mutex_unlock(&lock);

    close(new_socket);
    free(socket_desc);
    return NULL;
}

int main() {
    int server_fd, new_socket, *new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Initialize semaphore with value 1
    sem_init(&sem, 0, 1);

    // Initialize mutex
    pthread_mutex_init(&lock, NULL);

    // Create threads for both robot arms
    pthread_create(&thread1, NULL, robot_arm_1, NULL);
    pthread_create(&thread2, NULL, robot_arm_2, NULL);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Define the address and port for the server
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for client connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is waiting for connections...\n");
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        new_sock = malloc(sizeof(int));
        *new_sock = new_socket;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void*)new_sock) < 0) {
            perror("could not create thread");
            exit(EXIT_FAILURE);
        }

        // Detach the thread so that it cleans up after itself
        pthread_detach(client_thread);
    }

    // Destroy semaphore and mutex
    sem_destroy(&sem);
    pthread_mutex_destroy(&lock);

    return 0;
}
