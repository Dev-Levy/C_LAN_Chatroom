#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "network.h"

#define BUFFER_SIZE 100



struct MessageQueue 
{
    ChatMessage messages[BUFFER_SIZE];
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
};


int network_init(MessageQueue **queue) 
{
    *queue = malloc(sizeof(MessageQueue));
    if (*queue == NULL) 
    {
        return -1;
    }

    memset((*queue)->messages, 0, sizeof(ChatMessage) * BUFFER_SIZE);
    
    (*queue)->count = 0;
    
    if (pthread_mutex_init(&(*queue)->lock, NULL) != 0) 
    {
        free(*queue);
        return -1;
    }
    
    return 0;
}

int network_send_message(MessageQueue* queue, const char* sender, const char* message)
{
    if (!queue || !sender || !message) 
    {
        fprintf(stderr, "Error: Null parameter passed to network_send_message\n");
        return -2;
    }

    if (pthread_mutex_lock(&queue->lock) != 0) {
        perror("Failed to lock mutex");
        return -2;
    }

    ChatMessage *msg = &queue->messages[queue->count];

    strncpy(msg->sender, sender, MAX_SENDER_LEN - 1);
    msg->sender[MAX_SENDER_LEN - 1] = '\0';

    strncpy(msg->message, message, MAX_MSG_LEN - 1);
    msg->message[MAX_MSG_LEN - 1] = '\0';

    msg->timestamp = time(NULL);

    queue->count++;

    // Unlock the mutex
    if (pthread_mutex_unlock(&queue->lock) != 0) {
        perror("Failed to unlock mutex");
    }

    return 0;
}

const ChatMessage* network_get_recent_messages(MessageQueue* queue, size_t* count)
{
    pthread_mutex_lock(&queue->lock);
    
    *count = (queue->count > 20) ? 20 : queue->count;
    
    size_t start_index = (queue->count > 20) ? (queue->count - 20) : 0;
    
    const ChatMessage *result = &queue->messages[start_index];
    
    pthread_mutex_unlock(&queue->lock);
    return result;
}

void network_cleanup(MessageQueue *queue){

}