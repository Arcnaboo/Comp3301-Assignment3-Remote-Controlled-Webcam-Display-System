/*Comp3301 Assignment 3
** s4382911_MemcachedLibrary.c
** 
**
** Prepared by Arda Akgur
** libraries used: video4linux,  CSFMLgui, lib memcached, lirc
** 3rd parth program: avconv
*/

#include "s4382911_MemcachedLibrary.h"

static char **arcKeys;

static void setkeys() {
    arcKeys = malloc(sizeof(char*) * 10);
    arcKeys[0] = strdup(KEY_THERE_IS_MESSAGE);
    arcKeys[1] = strdup(KEY_WHO_GOT_MESSAGE);
    arcKeys[2] = strdup(KEY_WCD_MESSAGE);
    arcKeys[3] = strdup(KEY_IMG);
    arcKeys[4] = strdup(KEY_DELAY_RATE);
    arcKeys[5] = strdup(KEY_X);
    arcKeys[6] = strdup(KEY_Y);
    arcKeys[7] = strdup(KEY_W);
    arcKeys[8] = strdup(KEY_H);
    arcKeys[9] = strdup(KEY_SELECTED);
}

/*
** getKeys()
** Returns the list of current keys used by the library
*/
char **getKeys(void) {
    char **result = malloc(sizeof(char*) * 10);
    int i;
    for (i = 0; i < 10; i++) {
        result[i] = strdup(arcKeys[i]);
    }
    return result;
}

/*
** is_memc_success(MemcServer *ms)
** Returns true if previoud memc call was success
*/
bool is_memc_success(MemcServer *ms) {
    if (ms->rc != MEMCACHED_SUCCESS) {
        return false;
    }
    return true;
}

/*
** set_value_to_memc()
** Sets the given value to corresponding key 
** returns true if succesful
*/
bool m_set_value(MemcServer *ms, char *key, char *value) {
    
    ms->rc = memcached_set(ms->memc, key, strlen(key), value, strlen(value), (time_t)0, (uint32_t)0);
    return is_memc_success(ms);
}


/*
** get_value_from_memc()
** Gives the key to memc server and reads the value
** Saves the value in MemcValue struct 
** returns MemcValue struct if succesful
** else returns NULL
*/
MemcValue m_get_value(MemcServer *ms, char *key) {
    MemcValue result;
    result.retrievedValue = memcached_get(ms->memc, key, 
            strlen(key), &result.valueLength, &result.flags, &ms->rc);
    
    return result;
}
/*
** setup_server()
** sets up the memcached server 
** returns true if succesful
*/
bool setup_server(MemcServer *master) {
    setkeys();
    master->servers = NULL;
    master->memc = memcached_create(NULL);
    master->servers = memcached_server_list_append(master->servers, "localhost", 11211, &master->rc);
    master->rc = memcached_server_push(master->memc, master->servers);
    
    return is_memc_success(master);
}

/*
** set_default_variables()
** Sets the dafault values to memcached server
** function should be called after setting up the server 
** returns true if succesful
*/
bool set_default_variables(MemcServer *master) {
    
    if (arcKeys == NULL) return false;
    int i;
    for (i = 0; i < 10; i++) {
        if (!m_set_value(master, arcKeys[9], DEFAULT_VALUE)) {
            return false;
        }
    }
    return m_set_value(master, arcKeys[0], NO);
}

/*
** new_message()
** Sets so that the proccesses understand that there is new message
** function should be called after setting up a message
*/
void new_message(MemcServer *master) {
    (void) m_set_value(master, KEY_THERE_IS_MESSAGE, YES);
}

/*
** no_message()
** Sete so that the message has been read and 
** there is now new message this function should be called
** after a procees reads the message
*/
void no_message(MemcServer *master) {
    (void) m_set_value(master, KEY_THERE_IS_MESSAGE, NO);
}

/*
** is_there_new_message()
** returns true iff previously a process sets a message
** and calls new_message() else this method would return false
*/
bool is_there_new_message(MemcServer *master) {
    MemcValue mv = m_get_value(master, KEY_THERE_IS_MESSAGE);
    if (!strcmp(mv.retrievedValue, YES)) {
        free(mv.retrievedValue);
        return true;
    } else {
        free(mv.retrievedValue);
        return false;
    }
}

/*
** message_for_who()
** Sets the destination of next message 
** parameter who shouldbe WCDDISPLAY/WCDWEBCAM/WCDBOTH
*/
void message_for_who(MemcServer *master, char *who) {
    (void) m_set_value(master, KEY_WHO_GOT_MESSAGE, who);
}

/*
** who_received_message()
** Returns string value of who received next message
*/
char *who_received_message(MemcServer *master) {
    MemcValue mv = m_get_value(master, KEY_WHO_GOT_MESSAGE);
    char *ptr =strdup(mv.retrievedValue);
    free(mv.retrievedValue);
    return ptr;
}

/*
** decode_wcd_message()
** Returns message type of current message
*/
MessageType decode_wcd_message(MemcServer *master) {
    MemcValue mv = m_get_value(master, KEY_WCD_MESSAGE);
    if (!strcmp(mv.retrievedValue, WCD_START)) {
        free(mv.retrievedValue);
        return WCDSTART;
    }
    if (!strcmp(mv.retrievedValue, WCD_IMG)) {
        free(mv.retrievedValue);
        return WCDIMG;
    }
    if (!strcmp(mv.retrievedValue, WCD_CLEAR)) {
        free(mv.retrievedValue);
        return WCDCLEAR;
    }
    if (!strcmp(mv.retrievedValue, WCD_LIST)) {
        free(mv.retrievedValue);
        return WCDLIST;
    }
    if (!strcmp(mv.retrievedValue, WCD_KILL)) {
        free(mv.retrievedValue);
        return WCDKILL;
    }
    if (!strcmp(mv.retrievedValue, WCD_SELECT)) {
        free(mv.retrievedValue);
        return WCDSELECT;
    }
    if (!strcmp(mv.retrievedValue, WCD_TEXT_ENTRY)) {
        free(mv.retrievedValue);
        return WCDTEXT;
    }
    if (!strcmp(mv.retrievedValue, SHELL_CLEAR_CACHE)) {
        free(mv.retrievedValue);
        return SHELLCLEAR;
    }
    if (!strcmp(mv.retrievedValue, SHELL_GET_CACHE)) {
        free(mv.retrievedValue);
        return SHELLGET;
    }
    if (!strcmp(mv.retrievedValue, WCD_KILL_ALL)) {
        free(mv.retrievedValue);
        return KILLALL;
    }
    if (!strcmp(mv.retrievedValue, WCD_CLEAR_ALL)) {
        free(mv.retrievedValue);
        return CLEARALL;
    }
    if (!strcmp(mv.retrievedValue, WCD_MOVE)) {
        free(mv.retrievedValue);
        return WCDMOVE;
    }
    free(mv.retrievedValue);
    return WCDINVALID;
}

void set_the_cache(MemcServer *master, char *cache) {
    (void) m_set_value(master, KEY_CACHE, cache);
}

char *read_cache(MemcServer *master) {
    MemcValue mv = m_get_value(master, KEY_CACHE);
    char *ptr = strdup(mv.retrievedValue);
    free(mv.retrievedValue);
    return ptr;
}

void update_cache_length(MemcServer *master, int newLength) {
    char buf[10];
    sprintf(buf, "%d", newLength);
    (void) m_set_value(master, KEY_CACHE_LENGTH, buf);
}

int get_cache_length(MemcServer *master) {
    MemcValue mv = m_get_value(master, KEY_CACHE_LENGTH);
    int i;
    if (strlen(mv.retrievedValue) > 1) {
        return -1;
    }
    sscanf(mv.retrievedValue, "%d", &i);
    free(mv.retrievedValue);
	return i;
    
}

void set_the_move_direction(MemcServer *master, MoveDirection direction) {
    char c = '0' + direction;
    char cc[3];
    sprintf(cc, "%c", c);
    (void) m_set_value(master, KEY_DIRECTION, cc);
}

MoveDirection get_the_move_direction(MemcServer *master) {
    MemcValue mv = m_get_value(master, KEY_CACHE_LENGTH);
    int i;
    if (strlen(mv.retrievedValue) > 1) {
        return (MoveDirection)-1;
    }
    sscanf(mv.retrievedValue, "%d", &i);
    free(mv.retrievedValue);
	return (MoveDirection)i;
}

void clear_the_cache(MemcServer *master) {
    (void) m_set_value(master, KEY_CACHE, "");
}

/*
** set_the_message()
** Sets the next WCD message
*/
void set_the_message(MemcServer *master, char *message) {
    (void) m_set_value(master, KEY_WCD_MESSAGE, message);
}

/*
** set_the_start_info()
** Sets the next WCD start info
*/
void set_the_start_info(MemcServer *master, int x, int y, int w, int h) {
    char xx[20], yy[20], ww[20], hh[20];
    sprintf(xx, "%d", x);
    sprintf(yy, "%d", y);
    sprintf(ww, "%d", w);
    sprintf(hh, "%d", h);
    (void) m_set_value(master, KEY_X, xx);
    (void) m_set_value(master, KEY_Y, yy);
    (void) m_set_value(master, KEY_W, ww);
    (void) m_set_value(master, KEY_H, hh);
}

/*
** get_start_info()
** Returns pointer to a start info struct 
** struct contains next wcd start information
*/
WcdStartInfo *get_start_info(MemcServer *master) {
    WcdStartInfo *wsi = (WcdStartInfo*) malloc(sizeof(WcdStartInfo));
    MemcValue mv;
    mv = m_get_value(master, KEY_X);
    wsi->x = atoi(mv.retrievedValue);
    mv = m_get_value(master, KEY_Y);
    wsi->y = atoi(mv.retrievedValue);
    mv = m_get_value(master, KEY_W);
    wsi->w = atoi(mv.retrievedValue);
    mv = m_get_value(master, KEY_H);
    wsi->h = atoi(mv.retrievedValue);
    free(mv.retrievedValue);
    return wsi;
}

/*
** get_selected()
** Returns index value of selected WCD
*/
int get_selected(MemcServer *master) {
	MemcValue mv = m_get_value(master, KEY_SELECTED);
    int i;
    if (strlen(mv.retrievedValue) > 1) {
        return -1;
    }
    sscanf(mv.retrievedValue, "%d", &i);
    free(mv.retrievedValue);
	return i;
}

/*
** select_wcd()
** Selects the wcd to be manipulated on
*/
void select_wcd(MemcServer *master, char id) {
    char text[5];
    sprintf(text, "%c", id);
    (void) m_set_value(master, KEY_SELECTED, text);
}

/*
** set_img_message()
** Sets the next img manipulation type
*/
void set_img_message(MemcServer *master, ImgType type) {
    char text[50];
    
    switch (type) {
        case IMGINVALID:
            return;
        case IMGSTART:
            sprintf(text,"%s", IMG_START);
            break;
            
        case IMGRAW:
            sprintf(text, "%s", IMG_RAW);
            break;
        case IMGFLP:
            sprintf(text, "%s", IMG_FLP);
            break;
        case IMGDLY:
            sprintf(text, "%s", IMG_DLY);
            break;
        case IMGBLR:
            sprintf(text, "%s", IMG_BLR);
            break;
        case IMGARC:
            sprintf(text, "%s", IMG_ARC);
            break;
        
    }
    (void) m_set_value(master, KEY_IMG, text);
}

/*
** get_img_type()
** Returns the next img manipulation request type
*/
ImgType get_img_type(MemcServer *master) {
    MemcValue mv = m_get_value(master, KEY_IMG);
    if (!strcmp(mv.retrievedValue, IMG_START)) {
        free(mv.retrievedValue);
        return IMGSTART;
    }
    if (!strcmp(mv.retrievedValue, IMG_RAW)) {
        free(mv.retrievedValue);
        return IMGRAW;
    }
    if (!strcmp(mv.retrievedValue, IMG_FLP)) {
        free(mv.retrievedValue);
        return IMGFLP;
    }
    if (!strcmp(mv.retrievedValue, IMG_BLR)) {
        free(mv.retrievedValue);
        return IMGBLR;
    }
    if (!strcmp(mv.retrievedValue, IMG_DLY)) {
        free(mv.retrievedValue);
        return IMGDLY;
    }
    if (!strcmp(mv.retrievedValue, IMG_ARC)) {
        free(mv.retrievedValue);
        return IMGARC;
    }
    

    free(mv.retrievedValue);
    return IMGINVALID;
}

/*
** set_delay_rate()
** Sets the delay rate for image manipulation
*/
void set_delay_rate(MemcServer *master, int delay) {
    char text[20];
    sprintf(text, "%d", delay);
    (void) m_set_value(master, KEY_DELAY_RATE, text);
}

/*
** receive_delay_rate()
** returns the previously set img delay value
*/
int receive_delay_rate(MemcServer *master) {
    MemcValue mv = m_get_value(master, KEY_DELAY_RATE);
    int delay;
    sscanf(mv.retrievedValue, "%d", &delay);
    free(mv.retrievedValue);
    return delay;
}
