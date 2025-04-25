// chat_client.c - Simple user-space application to interact with the chat device
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define DEVICE_PATH "/dev/chatdb"
#define MAX_MSG_SIZE 256
#define USERNAME_SIZE 32

// Function prototypes
void send_message(int fd, const char* username, const char* message);
void read_messages(int fd);
void print_help();

int main() {
    int fd;
    char username[USERNAME_SIZE] = {0};
    char message[MAX_MSG_SIZE] = {0};
    char command[10] = {0};
    
    // Open the device
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return -1;
    }
    
    printf("Welcome to ChatDB Client!\n");
    printf("Enter your username: ");
    fgets(username, USERNAME_SIZE - 1, stdin);
    username[strcspn(username, "\n")] = 0; // Remove newline
    
    print_help();
    
    while (1) {
        printf("\nCommand > ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0; // Remove newline
        
        if (strcmp(command, "send") == 0) {
            printf("Enter message: ");
            fgets(message, MAX_MSG_SIZE - 1, stdin);
            message[strcspn(message, "\n")] = 0; // Remove newline
            send_message(fd, username, message);
        }
        else if (strcmp(command, "read") == 0) {
            read_messages(fd);
        }
        else if (strcmp(command, "help") == 0) {
            print_help();
        }
        else if (strcmp(command, "exit") == 0) {
            break;
        }
        else {
            printf("Unknown command. Type 'help' for available commands.\n");
        }
    }
    
    close(fd);
    return 0;
}

void send_message(int fd, const char* username, const char* message) {
    char buffer[MAX_MSG_SIZE + 64] = {0};
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    
    // Format timestamp as YYYY-MM-DD HH:MM:SS
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    
    // Format buffer as username:timestamp:message
    snprintf(buffer, sizeof(buffer), "%s:%s:%s", username, timestamp, message);
    
    if (write(fd, buffer, strlen(buffer)) < 0) {
        perror("Failed to send message");
    } else {
        printf("Message sent successfully.\n");
    }
}

void read_messages(int fd) {
    char buffer[MAX_MSG_SIZE + 64];
    ssize_t bytes_read;
    off_t pos = 0;
    
    printf("\n----- Chat Messages -----\n");
    
    // Move to the beginning of the device
    lseek(fd, 0, SEEK_SET);
    
    // Read all messages
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
    
    printf("------------------------\n");
    
    if (bytes_read < 0) {
        perror("Failed to read messages");
    }
}

void print_help() {
    printf("\nAvailable commands:\n");
    printf("send - Send a new message\n");
    printf("read - Read all messages\n");
    printf("help - Display this help information\n");
    printf("exit - Exit the application\n");
}
