#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


#include "main.h"
#include "network.h"

#define MSG_BUFFER_SIZE 20
#define MSG_SIZE 1024
#define STOP_COMMAND "stop"
#define CLEAR_LINE_ES "\33[2K\r"

int main(int argc, char *argv[]) {
    char username[MAX_SENDER_LEN] = {0};
    char command[10] = {0};
    printf("Enter your username: ");
    fgets(username, MAX_SENDER_LEN - 1, stdin);
    username[strcspn(username, "\n")] = '\0'; // Remove newline


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
    while (1) {
        
        
        ChatMessage message;
        strncpy(message.sender, username, MAX_SENDER_LEN);
        message.sender[MAX_SENDER_LEN] = '\0';
        
        read_message(message.message);
        
        if (strcmp(message.message, "exit") == 0) {
            break;
        }
        send_to_all(message);
        display_recent_messages();
        
    }
    
    shutdown_app();
    return 0;
}

void cli_init()
{
    system("clear");
    setCursorPosition(0, 3);
    printf("\033[2J\033[H");  // Clear screen
    
    // Header
    printf("\033[1;34m=== Chat History (Latest %d) ===\033[0m\n", MSG_BUFFER_SIZE);
    printf("\033[1m%-19s | %-11s | Message\033[0m\n", "Timestamp", "Sender");
    printf("------------------------------------------------\n");
}

// In main.c

void display_recent_messages() {

    cli_init();

    // Get and display messages
    int total = get_count();
    ChatMessage* msgs = network_get_messages();
    int start = total > MSG_BUFFER_SIZE ? total - MSG_BUFFER_SIZE : 0;

    for (int i = start; i < total; i++) {
        if (strlen(msgs[i].message) == 0) continue;
        if (i % 2 == 0) {
            printf("\033[37m");  // Light gray
        } else {
            printf("\033[90m");  // Dark gray
        }
        char clean_timestamp[20];
        strncpy(clean_timestamp, msgs[i].timestamp, 19);
        clean_timestamp[19] = '\0';
        
        printf("%-19s | %-15s | %s",
               clean_timestamp,
               msgs[i].sender,
               msgs[i].message);
    }

    // Input prompt
    printf("\n\033[1;32mYou > \033[0m");
    fflush(stdout);
    //clear_screen();
}


void read_message(char* input) {
    // Clear input line
    printf("\033[2K\r");  // Clear entire line
    
    // Show prompt
    printf("\033[1;32mYou > \033[0m");
    fflush(stdout);
    
    if (fgets(input, MAX_MSG_LEN, stdin) == NULL) {
        perror("Error reading input");
        return;
    }
    
    input[strcspn(input, "\n")] = '\0';
    
    // After reading, refresh messages
    display_recent_messages();
}

void setCursorPosition(int x, int y)
{
    printf("\033[%d;%dH", y, x);
}

