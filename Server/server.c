// Server side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

//#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_SOCKETS 30

pthread_mutex_t lock;
float counter = 0;
fd_set fds;
struct timeval timeout;

char *hello_message = "Hello from Server, you can send your requests";
char *OK_message = "HTTP/1.1 200 OK\r\n";
char *NOTFOUND_message = "HTTP/1.1 404 Not Found\r\n";
char buffer[BUFFER_SIZE] = {0};

int master_socket, addrlen, activity, valread, PORT;

struct sockaddr_in address;

void createMasterSocket();
int acceptConnection();
void closeConnection(int sd);
void sendMessage(int sd, char *msg);
void parseMessage(char *msg, char *type, char *path);
bool isCloseMessage(char *msg);
void receiveFile(char *path, int sd);
void sendFile(char *path, int sd);
void handlePostRequest(char *path, int sd);
void *handle_connection(void *);

int main(int argc, char const *argv[])
{
    // storing IP and Port of the server socket
    PORT = atoi(argv[1]);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    addrlen = sizeof(address);
    FD_ZERO(&fds);

    createMasterSocket();

    while (1)
    {
        int new_socket = acceptConnection();

        // increment counter of used sockets
        pthread_mutex_lock(&lock);
        FD_SET(new_socket, &fds);
        counter++;
        pthread_mutex_unlock(&lock);
        
        // delegate connection handling to a new thread
        pthread_t thread_id;
        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;
        pthread_create(&thread_id, NULL, handle_connection, pclient);
}

return 0;
}

/*
 * This will handle connection for each client
 * */

void createMasterSocket()
{
    int opt = 1;
    // create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // bind the socket to localhost and input port
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("\n[+] Listener on port %d \n", PORT);

    //  waits for the client to request initiating connection, 3-> max length of queue of pending connections

    // setmaximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // accept the incoming connection
    puts("\n[+] Waiting for connections ...");
}

int acceptConnection()
{
    int new_socket;
    if ((new_socket = accept(master_socket,
                             (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // inform user of socket number - used in send and receive commands
    printf("\n[+] New connection , socket fd is %d , ip is : %s , port : %d \n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    // send new connection greeting message
    if (send(new_socket, hello_message, strlen(hello_message), 0) != strlen(hello_message))
    {
        perror("send");
    }

    printf("\n[+] Welcome message sent successfully to client %d\n", new_socket);
    return new_socket;
}

void closeConnection(int sd)
{
    // Somebody disconnected , get his details and print
    getpeername(sd, (struct sockaddr *)&address,
                (socklen_t *)&addrlen);
    printf("\n[+] Host disconnected , ip %s , port %d \n",
           inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    // Close the sockect and mark as 0 in list for reuse
    close(sd);
}
void sendMessage(int sd, char *msg)
{
    if (send(sd, msg, strlen(msg), 0) != strlen(msg))
    {
        perror("send");
    }
}
void parseMessage(char *msg, char *type, char *path)
{
    char *t = strtok(msg, " ");
    char *p = strtok(NULL, " ");
    strcpy(type, t);
    strcpy(path, p);
}

bool isCloseMessage(char *msg)
{
    return (strstr(msg, "CLOSE") != NULL);
}

void receiveFile(char *path, int sd)
{
    FILE *file_pointer = fopen(path, "w");
    char *rcv_buffer, *file_buffer;
    rcv_buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    file_buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    int rcv_bytes;
    memset(rcv_buffer, '0', sizeof(rcv_buffer));
    while ((rcv_bytes = read(sd, rcv_buffer, BUFFER_SIZE)) > 0)
    {

        if ((fwrite(rcv_buffer, sizeof(char), rcv_bytes, file_pointer)) < rcv_bytes)
        {
            perror("File write failed! ");
            exit(EXIT_FAILURE);
        }
        strcpy(file_buffer, rcv_buffer);
        memset(rcv_buffer, '0', sizeof(rcv_buffer));

        if (rcv_bytes != 1024)
        {
            break;
        }
    }

    fclose(file_pointer);
    printf("\n[+] File Recieved Successfully.\n\n");
    sendMessage(sd, OK_message);
    // printf("File: %s\n\n", file_buffer);
}

void sendFile(char *path, int sd)
{
    FILE *fs;
    if ((fs = fopen(path, "r")) == 0)
    {
        sendMessage(sd, NOTFOUND_message);
        return;
    }
    sendMessage(sd, OK_message);
    printf("\n[+] Sending File %s...\n\n", path);

    char *rcv_buffer;
    int rcv_bytes;
    rcv_buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    while ((rcv_bytes = fread(rcv_buffer, sizeof(char), BUFFER_SIZE, fs)) > 0)
    {
        int send_bytes = send(sd, rcv_buffer, rcv_bytes, 0);
        if (send_bytes < 0)
        {
            perror("Failed to send file!");
            exit(EXIT_FAILURE);
        }

        memset(rcv_buffer, '0', sizeof(rcv_buffer));
        if (feof(fs))
        {
            free(rcv_buffer);
            break;
        }
    }
    printf("\n[+] File Sent Successfully.\n\n");
}

void handlePostRequest(char *path, int sd)
{
    sendMessage(sd, OK_message);
    receiveFile(path, sd);
}

void handleGetRequest(char *path, int sd)
{
    sendFile(path, sd);
}

void *handle_connection(void *socket_desc)
{
    pthread_mutex_lock(&lock);
    timeout.tv_sec =  10/ counter;
    pthread_mutex_unlock(&lock);
    // Get the socket descriptor
    int sd = *((int *)socket_desc);
    int read_size;
    char client_message[BUFFER_SIZE], *type = malloc(256), *path = malloc(256);

    while (1)
    {
        int t = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
        if (t == 0)
        {
            printf("\n[+] Timeout\n");
            closeConnection(sd);
            return 0;
        }
        read_size = read(sd, client_message, BUFFER_SIZE);
        if (read_size == -1)
        {
            perror("recv failed");
        }
        // sendOkMessage(sd);
        printf("\n[+] Client %d : %s \n", sd, client_message);
        if (isCloseMessage(client_message))
        {
            closeConnection(sd);
            break;
        }
        parseMessage(client_message, type, path);
        if (strcmp(type, "POST") == 0)
        {
            // printf("IN POST \n");
            handlePostRequest(path, sd);
        }
        else if (strcmp(type, "GET") == 0)
        {
            //  printf("IN GET \n");
            handleGetRequest(path, sd);
        }
        bzero(client_message, BUFFER_SIZE);
        bzero(type, 256);
        bzero(path, 256);
    }

    pthread_mutex_lock(&lock);
    counter--;
    timeout.tv_sec = 3 / counter;
    pthread_mutex_unlock(&lock);
    return 0;
}