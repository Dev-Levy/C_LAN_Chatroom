#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MSG_BUFFER_SIZE 20
#define MSG_SIZE 1024
#define STOP_COMMAND "stop"
#define CLEAR_LINE_ES "\33[2K\r"

void initScreen();
void setToWriteMsg();
void setCursorPosition(int x, int y);
void readAndStoreMsg(char* msg, char** msgs, int index);

int main()
{
    initScreen();
    
    int index = 0;
    char msg[MSG_SIZE];
    char* msgs[MSG_BUFFER_SIZE] = {NULL};
    
    do
    {
        setToWriteMsg();
        readAndStoreMsg(msg, msgs, index);
        index++;

        setCursorPosition(1,3);
        for (size_t i = 0; i < MSG_BUFFER_SIZE; i++)
        {
            int idx = (index + i) % MSG_BUFFER_SIZE;
            if (msgs[idx] != NULL)
            {
                printf("%s>>> %s", CLEAR_LINE_ES, msgs[idx]);
            }
        }
        
    } while (strcmp(msg, STOP_COMMAND) != 0);
    
    return 0;
}

void initScreen(){
    system("clear");
    printf("You:\n");
    printf("------------------------------------------");
}

void setToWriteMsg(){
    setCursorPosition(6,1);
    printf("%sYou: ", CLEAR_LINE_ES); //clears line and reprints You:
}

void readAndStoreMsg(char* msg, char** msgs, int index){
    char input[MSG_SIZE];
    fgets(input, sizeof(input), stdin);

    msgs[index % MSG_BUFFER_SIZE] = strdup(input);
    strcpy(msg, input);
}

void setCursorPosition(int x, int y){
    printf("\033[%d;%dH", y, x);
}