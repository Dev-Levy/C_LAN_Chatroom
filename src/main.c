#include <stdio.h>
#include <unistd.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <locale.h> // Add this for proper Unicode support

#include "main.h"
#include "network.h"

#define MSG_BUFFER_SIZE 10

// Color pairs
#define COLOR_HEADER 1
#define COLOR_USER_INFO 2
#define COLOR_MESSAGE_EVEN 3
#define COLOR_MESSAGE_ODD 4
#define COLOR_INPUT 5

// Window layout positions
#define HEADER_HEIGHT 6
#define INPUT_HEIGHT 3

WINDOW *header_win;
WINDOW *chat_win;
WINDOW *input_win;
WINDOW *status_win;

// Add near your other window declarations
static int cursor_y = 1;  // Default input line position
static int cursor_x = 8;  // After "You > " prompt

int main(int argc, char *argv[]) {
    // Set locale for proper Unicode support
    setlocale(LC_ALL, "");
    
    // Initialize NCurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    if (has_colors()) {
        start_color();
        init_color_pairs();
    }

    
    // Print program info
    printw("Program name: %s\n", argv[0]);
    
    if (argc == 1) {
        printw("No additional arguments provided.\n");
    } else {
        printw("Arguments:\n");
        for (int i = 1; i < argc; i++) {
            printw("%d: %s\n", i, argv[i]);
        }
    }
    refresh();
    
    // Initialize application
    init_app(argv[1]);
    
    // Get username
    echo();
    printw("Enter your username: ");
    ChatMessage message;
    getnstr(message.sender, MAX_SENDER_LEN - 1);
    noecho();
    
    // Setup UI
    cli_init();
    
    // Main loop
    display_recent_messages();
    read_message(message.message);
    
    while (strcmp(message.message, "exit") != 0) {
        send_to_all(message);
        display_recent_messages();
        read_message(message.message);
    }
    
    // Cleanup
    shutdown_app();
    delwin(header_win);
    delwin(chat_win);
    delwin(input_win);
    //delwin(status_win);
    endwin();
    return 0;
}

void init_color_pairs() {
    init_pair(COLOR_HEADER, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_USER_INFO, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_MESSAGE_EVEN, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_MESSAGE_ODD, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_INPUT, COLOR_WHITE, COLOR_BLUE);
}

void cli_init() {
    clear();
    refresh();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Create header window
    header_win = newwin(HEADER_HEIGHT, max_x, 0, 0);
    wbkgd(header_win, COLOR_PAIR(COLOR_HEADER));
    wattron(header_win, A_BOLD);
    
    // Draw header border
    box(header_win, 0, 0);
    
    // Header content
    mvwprintw(header_win, 1, 2, "┌───────────────────────────────────────┐");
    mvwprintw(header_win, 2, 2, "│ 'Simple' Chat Application             │");
    mvwprintw(header_win, 3, 2, "│ Connected Users:                      │");
    mvwprintw(header_win, 4, 2, "│ Last status message:                  │");
    mvwprintw(header_win, 5, 2, "└───────────────────────────────────────┘");
    
    wattroff(header_win, A_BOLD);
    wrefresh(header_win);
    
    // Create status window (inside header)
    //status_win = newwin(1, max_x - 30, 4, 25);
    //wbkgd(status_win, COLOR_PAIR(COLOR_USER_INFO));
    //wrefresh(status_win);
    
    // Create chat window
    chat_win = newwin(max_y - HEADER_HEIGHT - INPUT_HEIGHT, max_x, HEADER_HEIGHT, 0);
    scrollok(chat_win, TRUE);
    wrefresh(chat_win);
    
    // Create input window
    input_win = newwin(INPUT_HEIGHT, max_x, max_y - INPUT_HEIGHT, 0);
    wbkgd(input_win, COLOR_PAIR(COLOR_INPUT));
    mvwprintw(input_win, 1, 2, "You > ");
    wrefresh(input_win);
    
    // Initial user info display
    display_user_info(0);
}

void read_message(char* input) {
    curs_set(1);                // Show cursor
    noecho();                   // We'll handle echoing manually
    wmove(input_win, 1, 8);     // Start position
    wrefresh(input_win);

    int ch;
    int pos = 0;
    while ((ch = wgetch(input_win)) != '\n' && pos < MAX_MSG_LEN-1) {
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (pos > 0) {
                pos--;
                waddch(input_win, '\b');
                waddch(input_win, ' ');
                waddch(input_win, '\b');
            }
        } else if (isprint(ch)) {
            input[pos++] = ch;
            waddch(input_win, ch);  // Manual echo
        }
        getyx(input_win, cursor_y, cursor_x);  // Update position
        wrefresh(input_win);
    }
    input[pos] = '\0';
    cursor_x = 8;
    cursor_y = 1;

    // Reset for next input
    wmove(input_win, 1, 8);
    wclrtoeol(input_win);
    mvwprintw(input_win, 1, 2, "You > ");
    wrefresh(input_win);
    
    curs_set(0);
}

void display_recent_messages() {
    werase(chat_win);

    int saved_y = cursor_y;
    int saved_x = cursor_x;
    
    ChatMessage* msgs = network_get_messages();
    int total = get_count();
    int start = total > MSG_BUFFER_SIZE ? total - MSG_BUFFER_SIZE : 0;
    //unsigned int start = total-10;
    // Print column headers
    wattron(chat_win, A_BOLD);
    wprintw(chat_win, "%-9s │ %-15s │ %-20s\n", "Timestamp", "Sender", "Message");
    wattroff(chat_win, A_BOLD);
    
    // Print messages
    for (int i = start; i < total; i++) {
        if (strlen(msgs[i].message) == 0) continue;
        // Alternate colors
        wattron(chat_win, COLOR_PAIR(i % 2 ? COLOR_MESSAGE_ODD : COLOR_MESSAGE_EVEN));
        wprintw(chat_win, "%-9s │ %-15s │ %-20s\n", 
               msgs[i].timestamp, 
               msgs[i].sender, 
               msgs[i].message
                                );
        wattroff(chat_win, COLOR_PAIR(i % 2 ? COLOR_MESSAGE_ODD : COLOR_MESSAGE_EVEN));
    }
    
    wrefresh(chat_win);

    cursor_y = saved_y;
    cursor_x = saved_x;
    wmove(input_win, cursor_y, cursor_x);
    wrefresh(input_win);
}

void display_user_info(int flag) {
    //werase(status_win);
    
    switch (flag) {
        case 1:
            //wprintw(status_win,"User Connected!");
            mvwprintw(header_win, 4, 25, "%s","User Connected!");
            break;
        case 2:
            //wprintw(status_win, "User Disconnected!");
            mvwprintw(header_win, 4, 25, "%s","User Disconnected!");
            break;
        default:
            //wprintw(status_win,"Ready");
            mvwprintw(header_win, 4, 25, "%s","Ready");
            break;
    }
    
    // Update user count
    mvwprintw(header_win, 3, 20, "%d", get_accepted_count());
    
    //wrefresh(status_win);
    wrefresh(header_win);

    // FIX: Also return focus after status updates
    wmove(input_win, 1, 8);
    wrefresh(input_win);
}