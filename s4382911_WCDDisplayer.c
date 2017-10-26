/*Comp3301 Assignment 3
** s4382911_WCDDisplayer.c
** 
**
** Prepared by Arda Akgur
** libraries used: video4linux,  CSFMLgui, lib memcached, lirc
** 3rd parth program: avconv
*/

#include <stdio.h>
#include <stdlib.h>
#include <SFML/Graphics.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "s4382911_MemcachedLibrary.h"

#define HEIGHT                  480
#define WIDTH                   640



sfVideoMode VIDMODE = {320, 240, 32};
pthread_mutex_t jpeglock = PTHREAD_MUTEX_INITIALIZER;
static int selected = -1;

 typedef struct {
    
    bool startDisplay;
    sfRenderWindow *window;
    sfTexture *texture;
    sfSprite *sprite;
    sfEvent event;
} WebcamPlayer;

typedef struct {
    WcdStartInfo wsi;
    WebcamPlayer wp;
    int id;
    char chid;
    char *textToDisplay;
    bool textEntry;
} WCDData;


void setup_window(WebcamPlayer *displayer, WcdStartInfo *wcd, int id) {
    char wcdtitle[2500];
    sprintf(wcdtitle, "Comp3301 WCD - %d", id);
    displayer->window = sfRenderWindow_create(VIDMODE, wcdtitle, sfResize | sfClose, NULL);
    sfVector2i position;
    position.x = wcd->x;
    position.y = wcd->y;
    sfVector2u size;
    size.x = wcd->w;
    size.y = wcd->h;
    sfRenderWindow_setSize (displayer->window, size);
    sfRenderWindow_setPosition(displayer->window, position);
    displayer->sprite = sfSprite_create(); 
    sfRenderWindow_setFramerateLimit(displayer->window, 0);
}

void sss(int n) {
    
    printf("Terminating Display\n");
    exit(0);
}


void *displayer_thread(void *arg) {
    WCDData *wcddata = (WCDData*) arg;
    unsigned char frame[HEIGHT][WIDTH][3];
    setup_window(&wcddata->wp, &wcddata->wsi, wcddata->id);
    sleep(2);
    sfUint8 *pix = malloc(sizeof(sfUint8) * HEIGHT*WIDTH*4);
    while (sfRenderWindow_isOpen(wcddata->wp.window)) {
        // if there is action event then this run
        while (sfRenderWindow_pollEvent(wcddata->wp.window, &wcddata->wp.event)) {
            if (wcddata->wp.event.type == sfEvtClosed) {
                sfRenderWindow_close(wcddata->wp.window);
            }
        }
        // clear display
        sfRenderWindow_clear(wcddata->wp.window, sfBlack);
        fflush(stdin);
        fread(frame, HEIGHT*WIDTH*3, 1, stdin);
        
        int len = 0;
        
        for (int h = 0; h < HEIGHT; h++) {
            for (int w = 0; w < WIDTH; w++) {
                
                sfUint8 r = (sfUint8) frame[h][w][0];
                sfUint8 g = (sfUint8) frame[h][w][1];
                sfUint8 b = (sfUint8) frame[h][w][2];
                sfUint8 a = (sfUint8) 255; 
                pix[len++] = r;
                pix[len++] = g;
                pix[len++] = b;
                pix[len++] = a;
            }
        }
        
        //pthread_mutex_lock(&jpeglock);
        sfImage *img = sfImage_createFromPixels(WIDTH, HEIGHT, pix);
        wcddata->wp.texture = sfTexture_createFromImage(img, NULL);
        
        // set texture to sprite
        sfSprite_setTexture(wcddata->wp.sprite, wcddata->wp.texture, sfTrue);
        // draw sprite
        sfRenderWindow_drawSprite(wcddata->wp.window, wcddata->wp.sprite, NULL);
        // display window
        sfRenderWindow_display(wcddata->wp.window);
        sfTexture_destroy(wcddata->wp.texture);
        sfImage_destroy(img);
        
    }
    free(pix);
    sfSprite_destroy(wcddata->wp.sprite);
    sfRenderWindow_destroy(wcddata->wp.window);
    
    return NULL;
    
    
}

void print_my_info(WCDData *wcd) {
    sfVector2i wp = sfRenderWindow_getPosition(wcd->wp.window);
    sfVector2u ws = sfRenderWindow_getSize(wcd->wp.window);
    
    printf("WCDinfo- id:%c Position:(%d, %d) Size:(%u, %u)\n", wcd->chid, wp.x, wp.y, ws.x, ws.y);
}

// display id x y h w 
int main(int argc, char **argv) {
    
    signal(SIGKILL, sss);
    signal(SIGINT, sss);
    signal(SIGPIPE, sss);
    printf("Starting WCD Displayer  \n");
    WCDData *wcd = (WCDData*) malloc(sizeof(WCDData));
    wcd->wsi.x = atoi(argv[2]);
    wcd->wsi.y = atoi(argv[3]);
    wcd->wsi.w = atoi(argv[4]);
    wcd->wsi.h = atoi(argv[5]);
    wcd->chid = argv[1][0];
    sscanf(argv[1], "%d", &wcd->id);
    pthread_t disp;
    pthread_create(&disp, NULL,displayer_thread, wcd);
    pthread_detach(disp);
    MemcServer *master = (MemcServer*) malloc(sizeof(MemcServer));
    if (!setup_server(master)) {
        perror("Setting up MemcServer WCD");
        return 2;
    }  
    while (1) {
        if (is_there_new_message(master)) {
            char *who = who_received_message(master);
            if (strcmp(WCDDISPLAY, who) && strcmp(WCDBOTH, who)) continue;
            
            selected = get_selected(master);
            MessageType type = decode_wcd_message(master);
            
            if (type == WCDLIST) print_my_info(wcd);
     
            if (type == WCDKILL) {
                if (selected == wcd->id) {
                    sleep(1);
                    break;
                }
            }           
            if (type == KILLALL) {
                sleep(1);
                break;
                
            }
            
            if (type == WCDSELECT) {
                if (selected == wcd->id) {
                    sfRenderWindow_setActive(wcd->wp.window, sfTrue);
                }
            }
            
            no_message(master);
        }
    }   
    printf("Terminating WCD %d Display\n", wcd->id);
    return 0;   
}
