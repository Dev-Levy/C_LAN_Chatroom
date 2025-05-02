#ifndef SOCKETS_H
#define SOCKETS_H

#include "model.h"

int get_count();
int get_accepted_count();

void init_app(char* ip);
void send_to_all(ChatMessage msg);

ChatMessage* network_get_messages();
void shutdown_network(int serverSocketFD);
void shutdown_app();


#endif