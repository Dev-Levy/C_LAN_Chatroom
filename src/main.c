#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "main.h"
#include "network.h"

#define MSG_BUFFER_SIZE 20
#define MSG_SIZE 1024
#define STOP_COMMAND "stop"
#define CLEAR_LINE_ES "\33[2K\r"

int main(int argc, char *argv[]) {
    printf("Program name: %s\n", argv[0]);
    
    if (argc == 1) {
        printf("No additional arguments provided.\n");
    } else {
        printf("Arguments:\n");
        for (int i = 1; i < argc; i++) {
            printf("%d: %s\n", i, argv[i]);
        }
    }
    init_app(argv[1]);
    //NETWORK - server create, bind, listen
    //CHAR DEV - open, create DB

    cli_init();
    //cli setup
    ChatMessage message;
    read_message(message.message);
    printf("Message read: %s\n",message.message);
    // input char helyett ChatMessage lenne
    send_to_all(message);

    // while (1) 
    // {
    //     display_recent_messages(queue);
    //     read_and_send_message(queue);
    // }

    // network_cleanup(queue);
    // return 0;
}

void cli_init()
{
    system("clear");
    printf("You:\n");
    printf("------------------------------------------\n");
}

// void display_recent_messages() 
// {
//     size_t count;
//     const ChatMessage* messages = network_get_recent_messages(/*queue,*/ &count);
    
//     setCursorPosition(1,3);
    
//     for (size_t i = 0; i < count; i++) 
//     {
//         char time_buf[32];
//         strftime(time_buf, sizeof(time_buf), "%H:%M:%S", localtime(&messages[i].timestamp));
        
//         printf("%s[%s] %s: %s\n",
//             CLEAR_LINE_ES, 
//             time_buf,
//             messages[i].sender,
//             messages[i].message);
//     }
// }

void read_message(char* input) 
{    
    
    
    setCursorPosition(1,1);
    printf("%sYou: ", CLEAR_LINE_ES); //clears line and reprints You:
    
    if (fgets(input, MAX_MSG_LEN, stdin) == NULL) 
    {
        perror("Error reading input");
        
        return;
    }
    
    input[strcspn(input, "\n")] = '\0';
    
    return;
}

void setCursorPosition(int x, int y)
{
    printf("\033[%d;%dH", y, x);
}