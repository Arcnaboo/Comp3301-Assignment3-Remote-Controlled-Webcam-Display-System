/*Comp3301 Assignment 3
** s4382911_WCDRemoteControl.c
** 
**
** Prepared by Arda Akgur
** libraries used: video4linux,  CSFMLgui, lib memcached, lirc
** 3rd parth program: avconv
*/

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include "s4382911_MemcachedLibrary.h"         
/* Uninmplemented feature
static const char twos[] = {'2', 'a', 'b', 'c'};
static const char tres[] = {'3', 'd', 'e', 'f'};
static const char four[] = {'4', 'g', 'h', 'i'};
static const char five[] = {'5', 'j', 'k', 'l'};
static const char sixe[] = {'6', 'm', 'n', 'o'};
static const char seve[] = {'7', 'p', 'q', 'r', 's'};
static const char eiht[] = {'8', 't', 'u', 'v'};
static const char nine[] = {'9', 'w', 'x', 'y', 'z'};
static int nexttwo = 0;
static int nextthr = 0;
static int nextfor = 0;
static int nextfiv = 0;
static int nextsix = 0;
static int nextsev = 0;
static int nexteih = 0;
static int nextnin = 0;
static bool remoteTextEntry = false;

static clock_t textEntryStart;
static clock_t lastKeyPress;
*/

static char currentId;

static char lastRequestedStart;

int powerRequest = 2;

char get_next(const char array[], int length, int *next) {
    char out = array[*next];
    next[0]++;
    if (next[0] == length) {
        next[0] = 0;
    }
    return out;
}


typedef enum {
    K0,
    K1,
    K2,
    K3,
    K4,
    K5,
    K6,
    K7,
    K8,
    K9,
    POWER,
    MUTE,
    RECORD,
    TIME,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    INVALID
    
} RemoteKey;

RemoteKey decode_key(char *token) {
    if (!strcmp(token, KEY_0)) return K0;
    if (!strcmp(token, KEY_1)) return K1;
    if (!strcmp(token, KEY_2)) return K2;
    if (!strcmp(token, KEY_3)) return K3;
    if (!strcmp(token, KEY_4)) return K4;
    if (!strcmp(token, KEY_5)) return K5;
    if (!strcmp(token, KEY_6)) return K6;
    if (!strcmp(token, KEY_7)) return K7;
    if (!strcmp(token, KEY_8)) return K8;
    if (!strcmp(token, KEY_9)) return K9;
    if (!strcmp(token, KEY_POWER)) return POWER;
    if (!strcmp(token, KEY_MUTE)) return MUTE;
    if (!strcmp(token, KEY_RECORD)) return RECORD;
    if (!strcmp(token, KEY_TIME)) return TIME;
    if (!strcmp(token, KEY_UP)) return UP;
    if (!strcmp(token, KEY_DOWN)) return DOWN;
    if (!strcmp(token, KEY_LEFT)) return LEFT;
    if (!strcmp(token, KEY_RIGHT)) return RIGHT;
    
    
    return INVALID;
}




void process_phone_key_prex(MemcServer *ms, RemoteKey key) {
    /*if (!remoteTextEntry) {
        remoteTextEntry = true;
        textEntryStart = clock();
        textlen = 0;
    }*/
    
    
    /*
    switch(key) {
        case K0:
        case K1:
        case K2:
            next = get_next(twos, 4, &nexttwo);
            break;
        case K3:
            next = get_next(tres, 4, &nextthr);
            break;
        case K4,
        case K5,
        case K6,
        case K7,
        case K8,
        case K9,
        case POWER,
        case MUTE,
        case RECORD,
        case TIME,
        case UP,
        case DOWN,
        case LEFT,
        case RIGHT:
        case INVALID:
            break;
        
    }*/
   /* char buff[1024];
    int len = get_cache_length(ms);
    char *currentCache = read_cache(ms);
    int i;
    for (i = 0; i < len; i++) {
        buff[i] = currentCache[i];
    }
    char next;*/
    
}

bool is_key_digit(RemoteKey key) {
    
    
    if (key == K0 || key == K1 || key == K2 ||
        key == K3 || key == K4 || key == K5 ||
        key == K6 || key == K7 || key == K8 || key == K9) {
        return true;        
    }
    return false;
}

bool is_key_tele_key(RemoteKey key) {
    if (key == K2 ||
        key == K3 || key == K4 || key == K5 ||
        key == K6 || key == K7 || key == K8 || key == K9) {
        return true;
    }
    return false;
}

bool is_key_wcd_id(RemoteKey key) {
    if (key == K0 || key == K1 || key == K2 ||
        key == K3) {
            return true;
        }
        return false;
}


void process_digits(MemcServer *ms, RemoteKey key) {
    
    if (is_key_wcd_id(key)) {
        currentId = '0' + key;
        printf("Remote Chosen %c\n", currentId);
        return;
    }
    
    message_for_who(ms, WCDWEBCAM);
    set_the_message(ms, WCD_IMG);
    
    select_wcd(ms, currentId);
    
    if (key == K4) set_img_message(ms, IMGRAW);
    if (key == K5) set_img_message(ms, IMGFLP);
    if (key == K6) set_img_message(ms, IMGBLR);
    
    if (key == K7) {
        set_img_message(ms, IMGDLY);
        set_delay_rate(ms, 1000);
    }
    
    if (key == K8) set_img_message(ms, IMGARC);
    
    printf("Requesting %d for %c\n", key, currentId);
    new_message(ms);
}

void process_select(MemcServer *ms) {
    printf("Remote selected %c\n", currentId);
    message_for_who(ms, WCDBOTH);
    set_the_message(ms, WCD_SELECT);
    select_wcd(ms, currentId);
    new_message(ms);
    
}

void process_start(MemcServer *ms) {
    if (lastRequestedStart == currentId) {
        printf("####### WARNING #######\n");
        printf("Press %d more times to confirm KILL all WCD\n", powerRequest);
        printf("####### WARNING #######\n");
        powerRequest--;
        if (powerRequest == 0) {
            message_for_who(ms, WCDBOTH);
            set_the_message(ms, WCD_KILL_ALL);
            new_message(ms);
            powerRequest = 2;
            printf("kill all request sent \n");
            return;
        }
        return;
    }
    lastRequestedStart = currentId;
    powerRequest = 2;
    printf("Requesting capture for %c\n", currentId);
    message_for_who(ms, WCDWEBCAM);
    set_the_message(ms, WCD_IMG);
    set_img_message(ms, IMGSTART);
    new_message(ms);
}

void process_clear(MemcServer *ms) {
    printf("Requesting clear for %c\n", currentId);
    message_for_who(ms, WCDWEBCAM);
    select_wcd(ms, currentId);
    set_the_message(ms, WCD_CLEAR);
    new_message(ms);
}

void process_clear_all(MemcServer *ms) {
    printf("Requesting clear ALL\n");
    message_for_who(ms, WCDWEBCAM);
    select_wcd(ms, currentId);
    set_the_message(ms, WCD_CLEAR);
    new_message(ms);
}

void process_move_left(MemcServer *ms) {
    printf("Requesting Move left for %c\n", currentId);
    message_for_who(ms, WCDWEBCAM);
    select_wcd(ms, currentId);
    set_the_message(ms, WCD_MOVE);
    set_the_move_direction(ms, M_LEFT);
    new_message(ms);
}

void process_move_right(MemcServer *ms) {
    printf("Requesting Move right for %c\n", currentId);
    message_for_who(ms, WCDWEBCAM);
    select_wcd(ms, currentId);
    set_the_message(ms, WCD_MOVE);
    set_the_move_direction(ms, M_RIGHT);
    new_message(ms);
}

void process_key_press(MemcServer *ms, RemoteKey key) {
    /*
    if (remoteTextEntry && !is_key_tele_key(key)) {
        remoteTextEntry = false;
    }
    if (remoteTextEntry && clock() - textEntryStart > 50) {
        message_for_who(ms, WCDSHELL);
        set_the_message(ms, SHELL_CLEAR_CACHE);
        new_message(ms);
    }*/
    
    if (is_key_digit(key)) process_digits(ms, key);
    
    if (key == RECORD) process_select(ms);
    
    if (key == POWER) process_start(ms);
    
    if (key == MUTE) process_clear_all(ms);
    
    if (key == TIME) process_clear(ms);
    
    if (key == LEFT) process_move_left(ms);
    
}


int main(int argc, char **argv) {
    
    MemcServer *ms = malloc(sizeof(MemcServer));
    if (!setup_server(ms)) {
            perror("Setting up MemcServer WCD");
            return 2;
    }
    clear_the_cache(ms);
    update_cache_length(ms, 0);
    FILE *reader = popen("irw", "r");
    char buffer[1024];
    const char delim[2] = " ";
    char *token;
    
    while (true) {
        if (!fgets(buffer, 1024, reader)) {
            perror("Cant receive IR data");
            exit(127);
        }
        token = strtok(buffer, delim);
        token = strtok(NULL, delim);
        token = strtok(NULL, delim);
        //printf("%s pressed\n", token);
        RemoteKey key = decode_key(token);
        process_key_press(ms, key);
        
        
    }
    
    
    return 0;
}