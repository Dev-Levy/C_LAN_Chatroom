#ifndef SOCKETS_H
#define SOCKETS_H

#include "model.h"


void init_app();
void send_to_all(char* msg);

//getter
void shutdown_network(int serverSocketFD);


#endif