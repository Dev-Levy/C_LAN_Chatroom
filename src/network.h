#ifndef SOCKETS_H
#define SOCKETS_H

#include "model.h"


void init_app(char* ip);
void send_to_all(ChatMessage msg);

ChatMessage* network_get_recent_messages();
void shutdown_network(int serverSocketFD);


#endif