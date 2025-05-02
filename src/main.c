#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/select.h>

#include "main.h"
#include "network.h"

#define MSG_BUFFER_SIZE 20
#define MSG_SIZE 1024

#define CLEAR_LINE "\33[2K\r"
#define CLEAR_SCREEN "\33[2J"
#define MOVE_HOME "\33[H"

#define BOLD "\033[1m"
#define NORMAL "\033[0m"
#define ITALIC "\033[3m"

#define ASCII_LINE "────────────────────────────────────────────────\n"
#define LIGHT_GRAY "\033[37m"
#define DARK_GRAY "\033[90m"

int main(int argc, char *argv[]) {
    
    system("clear");
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
    
    ChatMessage message;
    printf("Enter your username: ");
    fgets(message.sender, MAX_SENDER_LEN - 1, stdin);
    message.sender[strcspn(message.sender, "\n")] = '\0';
    
    cli_init();
    
    display_recent_messages();
    read_message(message.message);
    
    int flag;
    while (strcmp(message.message, "exit") != 0) {
        
        usleep(500000); //500ms
        display_recent_messages();
        send_to_all(message);
        flag = get_flag();
        display_user_info(flag);
        read_message(message.message);
    }

    int endline = get_count() > MSG_BUFFER_SIZE ? 29 : (get_count() + 9);
    setCursorPosition(1, endline);
    
    shutdown_app();
    return 0;
}

void cli_init()
{
    system("clear");
    
    // Header
    printf(ASCII_LINE);
    printf("### 'Simple' Chat Application\n");
    printf("###  Connected Users: \n");
    printf(ASCII_LINE);
    printf("%s%-9s | %-15s | %-20s%s\n", BOLD, "Timestamp", "Sender", "Message", NORMAL);
    printf(ASCII_LINE);
    printf("%sYou > %s\n", BOLD, NORMAL);    
    printf(ASCII_LINE);
}

void read_message(char* input) {

    setCursorPosition(7, 7);

    if (input_available())
    {
        printf(CLEAR_LINE);
        printf("%sYou > %s", BOLD, NORMAL);
        fflush(stdout);
     
        if (fgets(input, MAX_MSG_LEN, stdin) == NULL) {
            perror("Error reading input");
            return;
        }
        input[strcspn(input, "\n")] = '\0';
    }
    
}

void display_recent_messages() {

    ChatMessage* msgs = network_get_messages();

    int total = get_count();
    int start = total > MSG_BUFFER_SIZE ? total - MSG_BUFFER_SIZE : 0;

    setCursorPosition(1,9);

    for (int i = start; i < total; i++) {
        if (strlen(msgs[i].message) == 0) continue;
        if (i % 2 == 0)
            printf(LIGHT_GRAY);
        else
            printf(DARK_GRAY);
        
        printf("%s%-9s | %-15s | %-20s", 
                CLEAR_LINE,
                msgs[i].timestamp, 
                msgs[i].sender, 
                msgs[i].message);
        printf(NORMAL);
    }

    fflush(stdout);
}

void display_user_info(int flag){
        
    switch (flag) {
        case 1:
            printf("A User Connected!!!\n");
            break;
        case 2:
            printf("A User Disconnected!!!\n");
            break;
            default:
            break;
        }

    setCursorPosition(23, 3);
    printf("%d", get_accepted_count());
}

int input_available(){
    struct timeval tv = {0,0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) == 1;
}

void setCursorPosition(int x, int y)
{
    printf("\033[%d;%dH", y, x);
}

