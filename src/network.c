#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include "network.h"

#include "main.h"

#define MAX_CONNECTION_NUM 10
#define DEVICE_PATH "/dev/chatdb"

#pragma region structs
struct AcceptedSocket
{
    int acceptedSocketFD;
    struct sockaddr_in address;
};

struct recvargs {
    int socketFD;
    int socketIndex;
};

#pragma endregion

#pragma region method signatures

int init_network();
int init_char_dev();
void shutdown_network(int FD);
void shutdown_char_dev(int FD);
int createTCPIpv4Socket(void);
struct sockaddr_in* CreateIPv4Address(char* ip, int port);
struct AcceptedSocket* acceptIncomingConnection(int serverSocketFD);
void acceptNewConnectionAndReceiveAndPrintItsData(int serverSocketFD);
void* receiveAndPrintIncomingData(void *args);
void* startAcceptingIncomingConnections(void *args);
void receiveAndPrintIncomingDataOnSeparateThread(struct recvargs* arguments);
void sendReceivedMessageToTheOtherClients(char *buffer,int socketFD);
void startAcceptingIncomingConnectionsOnSeparateThread(int serverSocketFD);
void ConnectToIPs();

#pragma endregion



int server_FD;
int chardev_FD;
ChatMessage messages[MAX_MESSAGES];
int msg_counter = 0;
int init = false;
int flag;

//ennek kéne struct
int available_IPs_sockets[MAX_CONNECTION_NUM];
struct sockaddr_in* available_IPs_adresses[MAX_CONNECTION_NUM];
char *available_IPs_adresses_in_string[MAX_CONNECTION_NUM];
int available_IPs_idx = 0;

int disconnected_socket_indexes[MAX_CONNECTION_NUM];
int disconnected_sockets_index;

int accepted_socket_count = 0;


int get_count(){
    return msg_counter;
}
int get_accepted_count(){
    return accepted_socket_count;
}
int get_flag(){
    return flag;
}

void init_app(char* ip) {
    printf("Started initing network\n");
    server_FD = init_network(ip);
    printf("Started initing chardev \n");
    chardev_FD = init_char_dev();
    printf("Init successful\n");
}

void shutdown_app(){
    shutdown_network(server_FD);
    shutdown_char_dev(chardev_FD);
}

int init_network(char* ip) {

    int serverSocketFD = createTCPIpv4Socket();
    struct sockaddr_in *serverAddress = CreateIPv4Address("",50000);

    int opt = 1; //this code turns off TURN_WAIT so that the port won't linger after close
    setsockopt(serverSocketFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int result = bind(serverSocketFD,(struct sockaddr *)serverAddress, sizeof(*serverAddress));
    if(result == 0) //We want to tell the system that we need port 50000 for connections.
        printf("socket was bound successfully\n"); //If successful result is 0
    else {
        printf("socket failed to bound, probably because the port is already in use\n");
        printf("The last error message is: %s\n", strerror(errno));
        free(serverAddress);
        return -1;
    }

    int listenResult = listen(serverSocketFD,MAX_CONNECTION_NUM); //we are waiting for connections, but the max is 10
    if(listenResult == 0)
        printf("socket listening successfully\n");
    else {
        printf("socket listening was not successful\n");
        printf("The last error message is: %s\n", strerror(errno));
    }

    startAcceptingIncomingConnectionsOnSeparateThread(serverSocketFD);

    printf("Scouting Started\n");

    ConnectToIPs(ip); //loads ip's in an array

    printf("Scouting Done\n");

    init = true;

    free(serverAddress); //ez shady de jóvanazúgy
    return serverSocketFD;
}

int init_char_dev(){
    
    int fd = open(DEVICE_PATH, O_RDWR);

    if (fd == -1) {
        printf("Failed to open %s: %s\n", DEVICE_PATH, strerror(errno));
        return -1;
    }

    // Device opened successfully
    printf("Device opened with FD = %d\n", fd);
    return fd;
}

void shutdown_network(int server_FD)
{
    shutdown(server_FD,SHUT_RDWR);
    
    close(server_FD);
    for (int i = 0; i < available_IPs_idx; ++i) {
        close(available_IPs_sockets[i]);
        free(available_IPs_adresses[i]);
        free(available_IPs_adresses_in_string[i]);
    }
}

void shutdown_char_dev(int chardev_FD){
    close(chardev_FD);
}

void send_to_all(ChatMessage msg) {

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    // Format timestamp in userspace
    strftime(msg.timestamp, sizeof(msg.timestamp), "%H:%M:%S", t);
    
    // Format buffer as "timestamp|sender|message"
    char buffer[TIMESTAMP_SIZE + MAX_SENDER_LEN + MAX_MSG_LEN];
    snprintf(buffer, sizeof(buffer), "TIMESTAMP=%s|SENDER=%s|MESSAGE=%s",
             msg.timestamp,
             msg.sender,
             msg.message);
    
    for (int i = 0; i < available_IPs_idx; ++i) {

        if (available_IPs_sockets[i] == 0)
            continue;

        int connectionSocketFD = available_IPs_sockets[i];
        ssize_t amountWasSent =  send(connectionSocketFD, buffer, strlen(buffer), 0);

        // printf("sent %zd bytes\n",amountWasSent);
    }  
    //save to chardev
    if (write(chardev_FD, &buffer, strlen(buffer)) < 0) {
        perror("Failed to store message");
    }
}

ChatMessage* network_get_messages() {
    char msg_buffer[MAX_MSG_LEN + 64];
    lseek(chardev_FD, 0, SEEK_SET);  // Rewind to start
    
    // Reset message counter
    msg_counter = 0;
    
    // Seek to beginning and read all messages
    lseek(chardev_FD, 0, SEEK_SET);
    
    while (msg_counter < MAX_MESSAGES) {
        ssize_t bytes_read = read(chardev_FD, msg_buffer, sizeof(msg_buffer)-1);
        
        if (bytes_read <= 0) {
            break;  // No more messages or error
        }
        
        msg_buffer[bytes_read] = '\0';
        
        // Parse each message
        char *timestamp = strstr(msg_buffer, "TIMESTAMP=");
        char *sender = strstr(msg_buffer, "SENDER=");
        char *message = strstr(msg_buffer, "MESSAGE=");
        
        if (!timestamp || !sender || !message) {
            continue;  // Skip invalid format
        }
        
        timestamp += 10;  // Skip "TIMESTAMP="
        sender += 7;      // Skip "SENDER="
        message += 8;     // Skip "MESSAGE="
        
        // Find delimiters
        char *timestamp_end = strchr(timestamp, '|');
        char *sender_end = strchr(sender, '|');
        
        if (!timestamp_end || !sender_end) {
            continue;  // Skip malformed messages
        }
        
        // Null-terminate each field
        *timestamp_end = '\0';
        *sender_end = '\0';
        

        // Store in messages array
        strncpy(messages[msg_counter].timestamp, timestamp, TIMESTAMP_SIZE-1);
        strncpy(messages[msg_counter].sender, sender, MAX_SENDER_LEN-1);
        strncpy(messages[msg_counter].message, message, MAX_MSG_LEN-1);
        
        // Ensure null-termination
        messages[msg_counter].timestamp[TIMESTAMP_SIZE-1] = '\0';
        messages[msg_counter].sender[MAX_SENDER_LEN-1] = '\0';
        messages[msg_counter].message[MAX_MSG_LEN-1] = '\0';
        
        msg_counter++;
    }
    
    return messages;
}

void ConnectToIPs(char* own_ip) {
    char ip[INET_ADDRSTRLEN]; //scouting on port 50000

    for (int i = 1; i < 255; ++i) { //try to connect to everyone
        snprintf(ip, sizeof(ip), "192.168.100.%d", i);
        if (strcmp(ip, own_ip) == 0)
            continue;
        
        int connectionSocketFD = createTCPIpv4Socket();
        struct sockaddr_in *connectionAddress = CreateIPv4Address(ip,50000);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 50000;

        // Set send timeout (applies to connect too)
        if (setsockopt(connectionSocketFD, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("setsockopt");
        }

        int result = connect(connectionSocketFD,(struct sockaddr *)connectionAddress,sizeof (*connectionAddress));

        if(result == 0) {
            printf("connection was successful\n");
            char *copyip = strdup(ip);
            printf("%s\n",copyip);
            bool exist = false;
            for (int i = 0; i < available_IPs_idx; ++i) {
                if (strcmp(available_IPs_adresses_in_string[i],copyip) == 0) {
                    exist = true;
                }
            }
            if (!exist) {
                int* socketFDPointer = malloc(sizeof(int));
                *socketFDPointer = connectionSocketFD;
                    available_IPs_sockets[available_IPs_idx] = connectionSocketFD;
                    available_IPs_adresses[available_IPs_idx] = connectionAddress;
                    available_IPs_adresses_in_string[available_IPs_idx] = copyip;
                    available_IPs_idx++;
                    accepted_socket_count++;
                    struct recvargs *recvargs = malloc(sizeof(struct recvargs));
                    recvargs->socketFD = *socketFDPointer;
                    recvargs->socketIndex = available_IPs_idx-1;
                    receiveAndPrintIncomingDataOnSeparateThread(recvargs);
                }

            //free(copyip);
        }
        else {
            close(connectionSocketFD);
            //printf("connection was not successful\n");
            //printf("The last error message is: %s\n", strerror(errno));
        }

    }

}

void startAcceptingIncomingConnectionsOnSeparateThread(int serverSocketFD) {

    pthread_t id;
    int *socketFDPointer = malloc(sizeof(int));
    *socketFDPointer = serverSocketFD; //had to malloc because the thread starts slowv
    pthread_create(&id,NULL,startAcceptingIncomingConnections,socketFDPointer);
}

void* startAcceptingIncomingConnections(void *args) {
    //int connectedSocketFD;
    //struct sockaddr_in *connectedSocketAddress;
    int serverSocketFD = *(int *)args;
    while(true)
    {
        struct AcceptedSocket* clientSocket  = acceptIncomingConnection(serverSocketFD);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientSocket->address.sin_addr, client_ip, sizeof(client_ip));
        bool exist = false;
        //int idxOfFoundUser;
        for (int i = 0; i < available_IPs_idx; ++i) {
            if (strcmp(available_IPs_adresses_in_string[i],client_ip) == 0) {
                exist = true;
            }
        }
        if (!exist) {
            if (disconnected_sockets_index > 0) {
                int index = disconnected_socket_indexes[disconnected_sockets_index-1];
                available_IPs_sockets[index] = clientSocket->acceptedSocketFD;
                available_IPs_adresses[index] = malloc(sizeof(struct sockaddr_in));
                memcpy(available_IPs_adresses[index], &clientSocket->address, sizeof(struct sockaddr_in));
                //available_IPs_adresses[index] = &clientSocket->address;
                available_IPs_adresses_in_string[index] = strdup(client_ip);
                disconnected_sockets_index--;
                accepted_socket_count++;
                if (init) { //status message
                    flag = 1;
                }
                else {
                    printf("Accepted Connection\n");
                }
                struct recvargs *recvargs = malloc(sizeof(struct recvargs));
                recvargs->socketFD = clientSocket->acceptedSocketFD;
                recvargs->socketIndex = index;
                receiveAndPrintIncomingDataOnSeparateThread(recvargs);
            } else {
                available_IPs_sockets[available_IPs_idx] = clientSocket->acceptedSocketFD;
                available_IPs_adresses[available_IPs_idx] = malloc(sizeof(struct sockaddr_in));
                memcpy(available_IPs_adresses[available_IPs_idx], &clientSocket->address, sizeof(struct sockaddr_in));
                //available_IPs_adresses[available_IPs_idx] = &clientSocket->address;
                available_IPs_adresses_in_string[available_IPs_idx] = strdup(client_ip);
                available_IPs_idx++;
                accepted_socket_count++;
                if (init) { //status message
                    flag = 1;
                }
                else {
                    printf("Accepted Connection\n");
                }
                struct recvargs *recvargs = malloc(sizeof(struct recvargs));
                recvargs->socketFD = clientSocket->acceptedSocketFD;
                recvargs->socketIndex = available_IPs_idx-1;
                receiveAndPrintIncomingDataOnSeparateThread(recvargs);
            }
        }
        /*while (connect(connectedSocketFD,(struct sockaddr *)connectedSocketAddress,sizeof *connectedSocketAddress) != 0 && errno == EBADF) {
            printf("CounterConnection failed\n");
            printf("The last error message is: %s\n", strerror(errno));
            if (errno == EBADF) {
                connectedSocketFD = createTCPIpv4Socket();
                availableIPsSockets[idxOfFoundUser] = connectedSocketFD;
                connectedSocketAddress = CreateIPv4Address(client_ip,50000);
                available_IPs_adresses[idxOfFoundUser] = connectedSocketAddress;
            }
        }
        printf("CounterConnection successful\n");*/

    }
}

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in clientAddress ;
    socklen_t clientAddressSize = sizeof (struct sockaddr_in);
    int clientSocketFD = 0;
    while (clientSocketFD <= 0) {
        clientSocketFD = accept(serverSocketFD,(struct sockaddr *)&clientAddress,&clientAddressSize);
        //obtain the other side's filedescriptor that gives us info about them
    }

    struct AcceptedSocket* acceptedSocket = malloc(sizeof (struct AcceptedSocket));
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    //put this info into a struct that we made

    return acceptedSocket;
}

void receiveAndPrintIncomingDataOnSeparateThread(struct recvargs * argument) {

    pthread_t id;
    pthread_create(&id,NULL,receiveAndPrintIncomingData,argument);
}

void* receiveAndPrintIncomingData(void *args) {
    struct recvargs *recvargs = args;
    int socketIndex = recvargs->socketIndex;
    int socketFD = recvargs->socketFD;
    char buffer[1024];

    //printf("Socket: %d\n",socketFD);

    while (true) //here we want to receive the message of a certain socket
    {
        ssize_t  amountReceived = recv(socketFD,buffer,1024,0);

        if(amountReceived>0)
        {
            buffer[amountReceived] = 0; //if its successful we write it out
            // printf("Chardevfd %d\n",chardev_FD);
            if (write(chardev_FD, &buffer, strlen(buffer)) < 0) {
                perror("Failed to store message\n");
            } else {
                flag = 0;
            }
        }

        if(amountReceived==0) //here we detect a disconnect
        {
            //printf("Someone has disconnected :(\n");
            accepted_socket_count--; //deletion from the arrays
            char emptybuffer[16] = "";
            available_IPs_adresses_in_string[socketIndex] = strdup(emptybuffer);
            available_IPs_sockets[socketIndex] = 0;
            available_IPs_adresses[socketIndex] = 0;
            disconnected_socket_indexes[disconnected_sockets_index] = socketIndex;
            disconnected_sockets_index++;
            flag = 2;
            break;
        }
    }
    close(socketFD);
    return NULL;
}

int createTCPIpv4Socket(void) {
    return socket(AF_INET, SOCK_STREAM, 0);
}

struct sockaddr_in* CreateIPv4Address(char* ip, int port) {
    struct sockaddr_in  *address = malloc(sizeof(struct sockaddr_in)); //alloc mem for the address
    address->sin_family = AF_INET; //give address family ipv4
    address->sin_port = htons(port);  //give the address a port

    if(strlen(ip) ==0) //if there is no IP then listen to any ip
        address->sin_addr.s_addr = INADDR_ANY;
    else
        inet_pton(AF_INET,ip,&address->sin_addr.s_addr);
    //if there is than convert it to usable format than give it to the address

    return address;
}
