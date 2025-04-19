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

#define numberofconnsallowed 10

struct AcceptedSocket
{
    int acceptedSocketFD;
    struct sockaddr_in address;
    int error;
    bool acceptedSuccessfully;
};

struct recvargs {
    struct AcceptedSocket *acceptedSocket;
    int connectedSocketFD;
};

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

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD);
void acceptNewConnectionAndReceiveAndPrintItsData(int serverSocketFD);
void *receiveAndPrintIncomingData(void *args);

void *startAcceptingIncomingConnections(void *args);

void receiveAndPrintIncomingDataOnSeparateThread(struct recvargs* arguments);

void sendReceivedMessageToTheOtherClients(char *buffer,int socketFD);

int availableIPsSockets[numberofconnsallowed];
struct sockaddr_in *availableIPsAdresses[numberofconnsallowed];
char *availableIPsAdressesinstring[numberofconnsallowed];
int availableIPsidx = 0;
int acceptedSocketCount = 0;
char chardev[]; //char device

void ConnectToIPs() {
    char ip[INET_ADDRSTRLEN]; //scouting on port 50000

    for (int i = 1; i < 255; ++i) { //try to connect to everyone
        if (i == 50){continue;}
        snprintf(ip, sizeof(ip), "192.168.100.%d", i);
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
            availableIPsSockets[availableIPsidx] = connectionSocketFD;
            availableIPsAdresses[availableIPsidx] = connectionAddress;
            availableIPsAdressesinstring[availableIPsidx] = strdup(ip); //i still dont know what is the problem with this,
            availableIPsidx++;                                          //but it has to be duped bc of memory address issues :/
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
    pthread_create(&id,NULL,startAcceptingIncomingConnections,&serverSocketFD);
}

void* startAcceptingIncomingConnections(void *args) {
    int connectedSocketFD;
    struct sockaddr_in *connectedSocketAddress;
    int serverSocketFD = *(int *)args;

    while(true)
    {
        struct AcceptedSocket* clientSocket  = acceptIncomingConnection(serverSocketFD);
        if (!clientSocket->acceptedSuccessfully)
            continue;
        acceptedSocketCount++;
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientSocket->address.sin_addr, client_ip, sizeof(client_ip));
        bool exist = false;
        int idxoffounduser;
        for (int i = 0; i < availableIPsidx; ++i) {
            if (strcmp(availableIPsAdressesinstring[i],client_ip) == 0) {
                exist = true;
                connectedSocketFD = availableIPsSockets[i];
                connectedSocketAddress = availableIPsAdresses[i];
                idxoffounduser = i;
            }
        }
        if (!exist) {
            if (availableIPsidx == 0) {
                availableIPsSockets[availableIPsidx] = clientSocket->acceptedSocketFD;
                availableIPsAdresses[availableIPsidx] = &clientSocket->address;
                availableIPsAdressesinstring[availableIPsidx] = client_ip;
                availableIPsidx++;
            }
            else {
                availableIPsidx++;
                availableIPsSockets[availableIPsidx] = clientSocket->acceptedSocketFD;
                availableIPsAdresses[availableIPsidx] = &clientSocket->address;
                availableIPsAdressesinstring[availableIPsidx] = client_ip;
            }
            connectedSocketFD = createTCPIpv4Socket();
            connectedSocketAddress = CreateIPv4Address(client_ip,50000);
        }
            reconnect:
            int result = connect(connectedSocketFD,(struct sockaddr *)connectedSocketAddress,sizeof *connectedSocketAddress);

            if(result == 0)
                printf("CounterConnection successful\n");
            else {
                printf("CounterConnection failed\n");
                printf("The last error message is: %s\n", strerror(errno));
                if (errno == EBADF) {
                    connectedSocketFD = createTCPIpv4Socket();
                    availableIPsSockets[idxoffounduser] = connectedSocketFD;
                    connectedSocketAddress = CreateIPv4Address(client_ip,50000);
                    availableIPsAdresses[idxoffounduser] = connectedSocketAddress;
                    goto reconnect;
                }
        }


        struct recvargs recvargs;
        recvargs.acceptedSocket = clientSocket;
        recvargs.connectedSocketFD = connectedSocketFD;

        receiveAndPrintIncomingDataOnSeparateThread(&recvargs);
    }
}



void receiveAndPrintIncomingDataOnSeparateThread(struct recvargs * argument) {

    pthread_t id;
    pthread_create(&id,NULL,receiveAndPrintIncomingData,argument);
}

void* receiveAndPrintIncomingData(void *args) {
    struct recvargs *arguments = (struct recvargs *) args;
    int socketFD = arguments->acceptedSocket->acceptedSocketFD;
    int connectedSocketFD = arguments->connectedSocketFD;
    char buffer[1024];

    while (true) //here we want to receive the message of a certain socket
    {
        ssize_t  amountReceived = recv(socketFD,buffer,1024,0);

        if(amountReceived>0)
        {
            buffer[amountReceived] = 0; //if its successful we write it out
            printf("%s\n",buffer); //chardevice
        }

        if(amountReceived==0) //here we should detect a disconnect
        {
            printf("Someone has disconnected :(\n");
            acceptedSocketCount--;
            break;
        }
    }
    close(connectedSocketFD);
    close(socketFD);
}

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in clientAddress ;
    socklen_t clientAddressSize = sizeof (struct sockaddr_in);
retryacept:
    int clientSocketFD = accept(serverSocketFD,(struct sockaddr *)&clientAddress,&clientAddressSize);
    //obtain the other side's filedescriptor that gives us info about them

    if(clientSocketFD == -1) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddress.sin_addr, ip, sizeof(ip));
        printf("%s, %d\n",ip,ntohs(clientAddress.sin_port));
        usleep(1000);
        goto retryacept;
    }

    printf("accepted\n");

    struct AcceptedSocket* acceptedSocket = malloc(sizeof (struct AcceptedSocket));
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->acceptedSuccessfully = clientSocketFD>=0;
    //put this info into a struct that we made

    if(!acceptedSocket->acceptedSuccessfully) {
        //printf("The last error message is: %s\n", strerror(errno));
        acceptedSocket->error = clientSocketFD;
    }

    return acceptedSocket;
}

void readConsoleEntriesAndSendToOthers(int serverSocketFD) {
    char *name = NULL;
    size_t nameSize= 0;
    printf("please enter your name?\n");
    ssize_t  nameCount = getline(&name,&nameSize,stdin);
    name[nameCount-1]=0;

    printf("Scouting Started\n");

    startAcceptingIncomingConnectionsOnSeparateThread(serverSocketFD); //start accpeting incoming connections after host's blocking part
    ConnectToIPs(); //loads ip's in an array

    printf("Scouting Done\n");

    if (acceptedSocketCount<1) {
        printf("currently there are noone here :/\n");
        while (acceptedSocketCount < 1) {}
        printf("Someone has connected!\n");
    }

    char *line = NULL;
    size_t lineSize= 0;
    printf("type and we will send...(type exit to quit)\n");

    char buffer[1024];
    while(true)
    {
        if (acceptedSocketCount<1) {
            printf("currently there are noone here :/\n"); //turn off writing would be better
            while (acceptedSocketCount < 1) {}
            printf("Someone has connected!\n");
        }

        ssize_t  charCount = getline(&line,&lineSize,stdin);
        line[charCount-1]=0;

        sprintf(buffer,"%s:%s",name,line);

        if(charCount>0)
        {
            if(strcmp(line,"exit")==0) {
                break;
            }

            for (int i = 0; i < availableIPsidx; ++i) { //try to connect to everyone
                int connectionSocketFD = availableIPsSockets[i];
                ssize_t amountWasSent =  send(connectionSocketFD,
                                          buffer,
                                          strlen(buffer), 0);
            }
        }
    }
}

int main(int argc, char *argv[]) {

    /*if (argc < 2) {
        printf("Please provide your IP address with the program start");
        return 1;
    }*/

    //char *my_ip = argv[1]; we should get their ip so we can exclude it form search

    int serverSocketFD = createTCPIpv4Socket();
    struct sockaddr_in *serverAddress = CreateIPv4Address("",50000); //listen for anyone on port 2000

    int opt = 1; //this code turns off TURN_WAIT so that the port won't linger after close
    setsockopt(serverSocketFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int result = bind(serverSocketFD,(struct sockaddr *)serverAddress, sizeof(*serverAddress));
    if(result == 0) //We want to tell the system that we need port 50000 for connections.
        printf("socket was bound successfully\n"); //If successful result is 0
    else {
        printf("socket failed to bound, probably because the port is already in use\n");
        printf("The last error message is: %s\n", strerror(errno));
        free(serverAddress);
        return 1;
    }

    int listenResult = listen(serverSocketFD,numberofconnsallowed); //we are waiting for connections, but the max is 10
    if(listenResult == 0)
        printf("socket listening successfully\n");
    else {
        printf("socket listening was not successful\n");
        printf("The last error message is: %s\n", strerror(errno));
    }

    /*struct conacceptargs args;
    args.serverSocketFD = serverSocketFD;
    args.connectedSocketFD = connectionSocketFD;
    args.connectedSocketAddress = connectionAddress;*/

    readConsoleEntriesAndSendToOthers(serverSocketFD);

    shutdown(serverSocketFD,SHUT_RDWR);

    close(serverSocketFD);
    for (int i = 0; i < availableIPsidx; ++i) {
        close(availableIPsSockets[i]);
        free(availableIPsAdresses[i]);
        free(availableIPsAdressesinstring[i]);
    }
    free(serverAddress);
    return 0;
}
