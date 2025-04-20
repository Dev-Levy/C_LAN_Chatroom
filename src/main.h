#ifndef PRESENTATION_LAYER_H
#define PRESENTATION_LAYER_H

#include "network.h"

void cli_init();
void display_recent_messages(MessageQueue* queue);
void read_and_send_message(MessageQueue *queue);
void setCursorPosition(int x, int y);

#endif