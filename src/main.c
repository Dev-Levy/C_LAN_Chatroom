#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "network.h"

#define MSG_BUFFER_SIZE 20
#define MSG_SIZE 1024
#define STOP_COMMAND "stop"
#define CLEAR_LINE_ES "\33[2K\r"

int main()
{
    cli_init();

    MessageQueue *queue;
    if (network_init(&queue) != 0) 
    {
        printf("Network init failed");
    }

    while (1) 
    {
        display_recent_messages(queue);
        read_and_send_message(queue);
    }

    network_cleanup(queue);
    return 0;
}

void cli_init()
{
    system("clear");
    printf("You:\n");
    printf("------------------------------------------");
}

void display_recent_messages(MessageQueue *queue) 
{
    size_t count;
    const ChatMessage* messages = network_get_recent_messages(queue, &count);
    
    setCursorPosition(1,3);
    
    for (size_t i = 0; i < count; i++) 
    {
        char time_buf[32];
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", localtime(&messages[i].timestamp));
        
        printf("%s[%s] %s: %s\n",
            CLEAR_LINE_ES, 
            time_buf,
            messages[i].sender,
            messages[i].message);
    }
}

void read_and_send_message(MessageQueue *queue) 
{    
    char input[MSG_SIZE];
    
    setCursorPosition(1,1);
    printf("%sYou: ", CLEAR_LINE_ES); //clears line and reprints You:
    
    if (fgets(input, sizeof(input), stdin) == NULL) 
    {
        perror("Error reading input");
        return;
    }
    
    input[strcspn(input, "\n")] = '\0';
    
    if (network_send_message(queue, "current_user", input) != 0)
    {
        fprintf(stderr, "Failed to send message\n");
    }
}

void setCursorPosition(int x, int y)
{
    printf("\033[%d;%dH", y, x);
}