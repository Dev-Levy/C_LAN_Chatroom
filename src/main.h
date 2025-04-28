#ifndef PRESENTATION_LAYER_H
#define PRESENTATION_LAYER_H

#include "network.h"

void cli_init();
void display_recent_messages();
void read_message(char* input);
void setCursorPosition(int x, int y);

#endif