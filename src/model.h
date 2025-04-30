
#define MAX_SENDER_LEN 32
#define MAX_MSG_LEN 1024
#define TIMESTAMP_SIZE 20
#define MAX_MESSAGES 128

typedef struct {
    char sender[MAX_SENDER_LEN];
    char message[MAX_MSG_LEN];
    char timestamp[TIMESTAMP_SIZE];
} ChatMessage;


