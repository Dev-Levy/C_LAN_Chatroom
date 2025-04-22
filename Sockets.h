#ifndef SOCKETS_H
#define SOCKETS_H

struct AcceptedSocket
{
    int acceptedSocketFD;
    struct sockaddr_in address;
};

struct recvargs {
    int socketFD;
    int socketIndex;
};

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD);
void acceptNewConnectionAndReceiveAndPrintItsData(int serverSocketFD);
void *receiveAndPrintIncomingData(void *args);
void *startAcceptingIncomingConnections(void *args);
void receiveAndPrintIncomingDataOnSeparateThread(struct recvargs* arguments);
void sendReceivedMessageToTheOtherClients(char *buffer,int socketFD);
int createTCPIpv4Socket(void);
struct sockaddr_in* CreateIPv4Address(char* ip, int port);
void startAcceptingIncomingConnectionsOnSeparateThread(int serverSocketFD);
void readConsoleEntriesAndSendToOthers(int serverSocketFD);
int init_network();
void shutdownNetwork(int serverSocketFD);
void ConnectToIPs();

#endif //SOCKETS_H
