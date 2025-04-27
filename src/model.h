#define MAX_MESSAGES 128
#define MAX_SENDER_LEN 32
#define MAX_MSG_LEN 1024

typedef struct {
    char sender[MAX_SENDER_LEN];
    char message[MAX_MSG_LEN];
    time_t timestamp;
} ChatMessage;

typedef struct {
    ChatMessage messages[MAX_MESSAGES];
    int msg_count;
    struct mutex lock;
} ChatDatabase;
