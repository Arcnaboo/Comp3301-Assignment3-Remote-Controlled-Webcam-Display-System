/*Comp3301 Assignment 3
** s4382911_MemcachedLibrary.h
** 
**
** Prepared by Arda Akgur
** libraries used: video4linux,  CSFMLgui, lib memcached , lirc
** 3rd parth program: avconv
*/



#ifndef _ARC_MEMC_H
#define _ARC_MEMC_H

#include <libmemcached/memcached.h> /* Memcached*/
#include <stdio.h> /* For print*/
#include <string.h> /* For strlen strdup.. */
#include <stdbool.h> /* true false*/
#include <stdlib.h> /* Malloc*/

/*Yes No Strings*/
#define YES                     "yes"
#define NO                      "no"

/*WCD Message Type*/
typedef enum {
    WCDINVALID = -1,
    WCDSTART,
    WCDSELECT,
    WCDLIST,
    WCDIMG,
    WCDCLEAR,
    WCDKILL,
    WCDTEXT,
    SHELLCLEAR,
    SHELLGET,
    KILLALL,
    CLEARALL,
    WCDMOVE
} MessageType;

typedef enum {
    M_INVALID = -1,
    M_UP,
    M_DOWN,
    M_LEFT,
    M_RIGHT
} MoveDirection;


/*IMG Type*/
typedef enum {
    IMGINVALID = -1,
    IMGSTART,
    IMGRAW,
    IMGFLP,
    IMGBLR,
    IMGDLY,
    IMGARC
} ImgType;

/*Start infor of WCD*/
typedef struct {
    int x,y,w,h,id;
} WcdStartInfo;

// structs to be used 
typedef struct {
    
    char *retrievedValue;
    size_t valueLength;
    uint32_t flags;
} MemcValue;

// Memcached Information
typedef struct {
    memcached_server_st *servers;
  	memcached_st *memc;
  	memcached_return rc;
} MemcServer;

// default value to store at start
#define DEFAULT_VALUE           "default"

// memchaced keys

#define KEY_THERE_IS_MESSAGE    "ismessage"
#define KEY_WHO_GOT_MESSAGE     "messagewho"
#define KEY_WCD_MESSAGE         "wcdmessage"
#define KEY_SELECTED			"getselected"
#define KEY_IMG                 "imgkey"
#define KEY_DELAY_RATE          "drate"
#define KEY_X                   "keyx"
#define KEY_Y                   "keyy"
#define KEY_W                   "width"
#define KEY_H                   "height"
#define KEY_A_WCD_SELECTED      "wcdselected"
#define KEY_CACHE               "cache"
#define KEY_CACHE_LENGTH        "cachelen"
#define KEY_DIRECTION           "direction"

// who should receive the cache
#define WCDWEBCAM               "controller"
#define WCDDISPLAY              "displayer"
#define WCDBOTH                 "both"
#define WCDSHELL                "shell"

// actual cache 
#define WCD_START               "wcdstart"
#define WCD_IMG                 "img"
#define WCD_LIST                "list"
#define WCD_CLEAR               "clear"
#define WCD_KILL                "kill"
#define WCD_SELECT              "select"
#define WCD_TEXT_ENTRY          "textentry"
#define WCD_REMOTE_SELECT       "remoteselect"
#define SHELL_CLEAR_CACHE       "clearcache"
#define SHELL_GET_CACHE         "getcache"
#define WCD_KILL_ALL            "killall"
#define WCD_CLEAR_ALL           "clearall"
#define WCD_MOVE                "move"


// img types type values obtained by KEY_IMG
#define IMG_START               "imgstart"
#define IMG_RAW                 "raw"
#define IMG_FLP                 "flp"
#define IMG_BLR                 "blr"
#define IMG_DLY                 "dly"
#define IMG_ARC                 "arc"

// remote keys 

#define KEY_POWER               "KEY_POWER"
#define KEY_MUTE                "KEY_MUTE"
#define KEY_RECORD              "KEY_RECORD"
#define KEY_TIME                "KEY_TIME"
#define KEY_UP                  "KEY_CHANNELUP"
#define KEY_DOWN                "KEY_CHANNELDOWN"
#define KEY_LEFT                "KEY_VOLUMEDOWN"
#define KEY_RIGHT               "KEY_VOLUMEUP"
#define KEY_0                   "KEY_0"
#define KEY_1                   "KEY_1"
#define KEY_2                   "KEY_2"
#define KEY_3                   "KEY_3"
#define KEY_4                   "KEY_4"
#define KEY_5                   "KEY_5"
#define KEY_6                   "KEY_6"
#define KEY_7                   "KEY_7"
#define KEY_8                   "KEY_8"
#define KEY_9                   "KEY_9"

////////////////////////
// Function Prototypes//
////////////////////////

void set_the_move_direction(MemcServer *master, MoveDirection direction);


MoveDirection get_the_move_direction(MemcServer *master);

void set_the_cache(MemcServer *master, char *cache);

char *read_cache(MemcServer *master);

void update_cache_length(MemcServer *master, int newLength);

int get_cache_length(MemcServer *master);
void clear_the_cache(MemcServer *master) ;

/*
** getKeys()
** Returns the list of current keys used by the library
*/
char **getKeys(void);

/*
** is_memc_success(MemcServer *ms)
** Returns true if previoud memc call was success
*/
bool is_memc_success(MemcServer *ms);

/*
** set_value_to_memc()
** Sets the given value to corresponding key 
** returns true if succesful
*/
bool m_set_value(MemcServer *ms, char *key, char *value);

/*
** get_value_from_memc()
** Gives the key to memc server and reads the value
** Saves the value in MemcValue struct 
** returns MemcValue struct if succesful
** else returns NULL
*/
MemcValue m_get_value(MemcServer *ms, char *key);

/*
** setup_server()
** sets up the memcached server 
** returns true if succesful
*/
bool setup_server(MemcServer *master);

/*
** set_default_variables()
** Sets the dafault values to memcached server
** function should be called after setting up the server 
** returns true if succesful
*/
bool set_default_variables(MemcServer *master);

/*
** new_message()
** Sets so that the proccesses understand that there is new message
** function should be called after setting up a message
*/
void new_message(MemcServer *master);

/*
** no_message()
** Sete so that the message has been read and 
** there is now new message this function should be called
** after a procees reads the message
*/
void no_message(MemcServer *master);

/*
** is_there_new_message()
** returns true iff previously a process sets a message
** and calls new_message() else this method would return false
*/
bool is_there_new_message(MemcServer *master);

/*
** message_for_who()
** Sets the destination of next message 
** parameter who shouldbe WCDDISPLAY/WCDWEBCAM/WCDBOTH
*/
void message_for_who(MemcServer *master, char *who);

/*
** who_received_message()
** Returns string value of who received next message
*/
char *who_received_message(MemcServer *master);

/*
** decode_wcd_message()
** Returns message type of current message
*/
MessageType decode_wcd_message(MemcServer *master);

/*
** set_the_message()
** Sets the next WCD message
*/
void set_the_message(MemcServer *master, char *message);

/*
** set_the_start_info()
** Sets the next WCD start info
*/
void set_the_start_info(MemcServer *master, int x, int y, int w, int h);

/*
** get_start_info()
** Returns pointer to a start info struct 
** struct contains next wcd start information
*/
WcdStartInfo *get_start_info(MemcServer *master);

/*
** get_selected()
** Returns index value of selected WCD
*/
int get_selected(MemcServer *master);

/*
** select_wcd()
** Selects the wcd to be manipulated on
*/
void select_wcd(MemcServer *master, char id);

/*
** get_img_type()
** Returns the next img manipulation request type
*/
ImgType get_img_type(MemcServer *master);

/*
** set_img_message()
** Sets the next img manipulation type
*/
void set_img_message(MemcServer *master, ImgType type);

/*
** set_delay_rate()
** Sets the delay rate for image manipulation
*/
void set_delay_rate(MemcServer *master, int delay);

/*
** receive_delay_rate()
** returns the previously set img delay value
*/
int receive_delay_rate(MemcServer *master);
#endif
