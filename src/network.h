#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#define MAX_SENDER_LEN 50
#define MAX_MSG_LEN 1024

#include <stddef.h>
#include <time.h>

typedef struct {
    char sender[MAX_SENDER_LEN];
    char message[MAX_MSG_LEN];
    time_t timestamp;
} ChatMessage;

typedef struct MessageQueue MessageQueue;
int network_init(MessageQueue **queue);
int network_send_message(MessageQueue *queue, const char *sender, const char *message);
const ChatMessage *network_get_recent_messages(MessageQueue *queue, size_t *count);
void network_cleanup(MessageQueue *queue);

#endif