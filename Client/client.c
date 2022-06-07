// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <stdbool.h>
//#define PORT 8080
#define BUFFER_SIZE 1024
#define SMALL_SIZE 256
int server_socket = 0, read_size, PORT;
struct sockaddr_in serv_addr;
char server_message[BUFFER_SIZE] = {0};

void createRequest(char *req, char *type, char *path, char *header_lines, char *body);
void initiateConnection();
void readServerMessage();
void parseCommand(char *command, char *request, char *type, char *path);
bool isOK();
void sendFile(char *path);
void receiveFile(char *path);
void sendGetRequest(char* request, char *path);
void sendPostRequest(char* request, char *path);
void sendRequests();

int main(int argc, char const *argv[])
{
    PORT = atoi(argv[2]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    initiateConnection();
    sendRequests();

    return 0;
}

void createRequest(char *req, char *type, char *path, char *header_lines, char *body)
{
    sprintf(req, "%s %s HTTP/1.1\r\n %s\r\n %s", type, path, header_lines, body);
}

void initiateConnection()
{
    // convert port # from string to int

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        exit(EXIT_FAILURE);
    }
    printf("\n[+] Socket created.\n");

    // Convert IPv4 and IPv6 addresses from text to binary form
    // IP address of server = argv[1] -> (127.0.0.1)
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\n[+] Invalid address/ Address not supported \n");
    }
    if (connect(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n[+] Connection Failed \n");
        exit(EXIT_FAILURE);
    }
    printf("\n[+] Connection created.\n");
    readServerMessage();
    bzero(server_message, BUFFER_SIZE);
}

void readServerMessage()
{
    read_size = read(server_socket, server_message, BUFFER_SIZE);
    if (read_size == -1)
    {
        perror("recv failed");
    }
    printf("\n[+] Server: %s\n", server_message);
}

bool isOK()
{
    return (strstr(server_message, "OK") != NULL);
}

void receiveFile(char *path)
{
    FILE *file_pointer = fopen(path, "w");
    char *rcv_buffer , *file_buffer;
    rcv_buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    file_buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    int rcv_bytes;
    memset(rcv_buffer, '0', sizeof(rcv_buffer));
    while ((rcv_bytes = read(server_socket, rcv_buffer, BUFFER_SIZE)) > 0)
    {

        if ((fwrite(rcv_buffer, sizeof(char), rcv_bytes, file_pointer)) < rcv_bytes)
        {
            perror("\n[+] File write failed! ");
            exit(EXIT_FAILURE);
        }
        strcpy(file_buffer , rcv_buffer);
        memset(rcv_buffer, '0', sizeof(rcv_buffer));
        
        if (rcv_bytes != 1024)
        {
            break;
        }
    }
    fclose(file_pointer);
    printf("\n[+] File Recieved Successfully.\n\n");
}

void sendFile(char *path)
{
    printf("\n[+] Sending File %s...\n\n", path);
    FILE *fs;
    fs = fopen(path, "r");
    char *rcv_buffer;
    int rcv_bytes;
    rcv_buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    while ((rcv_bytes = fread(rcv_buffer, sizeof(char), BUFFER_SIZE, fs)) > 0)
    {
        int send_bytes = send(server_socket, rcv_buffer, rcv_bytes, 0);
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

void sendGetRequest(char* request, char *path )
{
    send(server_socket, request, strlen(request), 0);
    readServerMessage();
    if( isOK() ){
        receiveFile(path);
    }
}

void sendPostRequest(char* request, char *path)
{
    send(server_socket, request, strlen(request), 0);
    readServerMessage();
    if( isOK() ){
        sendFile(path);
        readServerMessage();
        if( !isOK() ){
            sendPostRequest(request,path);
        }
    }
    
}

void parseCommand(char *command, char *request, char *type, char *path)
{
    char *t = strtok(command, " ");
    char *p = strtok(NULL, " ");
    strcpy(path, p);
    if (strstr(t, "get") != NULL)
    {
        strcpy(type, "GET");
    }
    else
    {
        strcpy(type, "POST");
    }
    createRequest(request, type, path, "", "");
}

void sendRequests()
{
    // reading commands from file
    FILE *fp;
    char *line = NULL;
    char *token, type[SMALL_SIZE], *path = malloc(256);
    size_t len = 0;
    ssize_t r;
    char request[BUFFER_SIZE];

    fp = fopen("commands.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((r = getline(&line, &len, fp)) != -1)
    {
        parseCommand(line, request, type, path);
        if (strcmp(type, "POST") == 0)
        {   
            sendPostRequest(request , path);
        }
        else if (strcmp(type, "GET") == 0)
        {
            sendGetRequest(request , path);
        }

        bzero(server_message, BUFFER_SIZE);
        bzero(request, BUFFER_SIZE);
        bzero(type, SMALL_SIZE);
        bzero(path, SMALL_SIZE);
    }
    createRequest(request, "CLOSE", "", "", "");
    send(server_socket, request, strlen(request), 0);

    fclose(fp);
    if (line)
        free(line);
    exit(EXIT_SUCCESS);
}