/*Comp3301 Assignment 3
** s4382911_WCDBenchamarker.h
** 
** Note: Gui currently not working as intended
** 
** Prepared by Arda Akgur
** libraries used: video4linux,  CSFMLgui, lib memcached
** 3rd parth program: avconv
*/

#include <stdio.h>
#include <stdlib.h>
#include <SFML/Graphics.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

sfVideoMode VIDMODE = {320, 240, 32};

 typedef struct {
    
    sfSprite* sprite;
    sfRenderWindow *window;
    sfEvent event;
    sfText* text;
    sfTexture* texture;
} Displayer;

void setup_window(Displayer *displayer) {
    char wcdtitle[2500];
    sprintf(wcdtitle, "Comp3301 WCD - Benchmarker");
    displayer->window = sfRenderWindow_create(VIDMODE, wcdtitle, sfResize | sfClose, NULL);
    sfVector2i position;
    position.x = 500;
    position.y = 500;
    sfVector2u size;
    size.x = 400;
    size.y = 400;
    sfRenderWindow_setSize (displayer->window, size);
    sfRenderWindow_setPosition(displayer->window, position);
    sfRenderWindow_setFramerateLimit(displayer->window, 0);
    displayer->text = sfText_create();
    displayer->sprite = sfSprite_create();
    displayer->texture = sfTexture_createFromFile("comp.png", NULL);
    sfSprite_setTexture(displayer->sprite, displayer->texture, sfTrue);
}


char *get_data(void) {
    char *dataOut = malloc(sizeof(char) * 4096);
    int k = 0;
    int pid;
    FILE *pip = popen("ps -a", "r");
    const char delim[2] = " ";
    char buff[4096];
    while (fgets(buff, 4096, pip)) {
        buff[strlen(buff) - 1] = '\0';
        char *token =strtok(buff, delim);
        char *pidd = strdup(token);
        while (token != NULL) {
            // all my processes has start with 'wcd' that is why comparing wcd
            if (!strncmp(token, "wcd", 3)) {
                pid = atoi(pidd);
                char cmopen[50];
                sprintf(cmopen,"cat /proc/%d/status", pid);
                FILE *pip2 = popen(cmopen, "r");
                char bu[500];
                int count = 0;
                while (fgets(bu, 500, pip2)) { // these counts important
                    if (count == 0 || count == 2 ||
                        count == 3 || count == 6 ||
                        count == 16 || count == 17 ||
                        count == 23 || count == 25 ||
                        count == 32 ) {
                         strcat(dataOut, bu);
                        }
                    count++;
                    
                    if (count == 33) break; // no need to read extra
                }
                pclose(pip2);
            }
            token = strtok(NULL, delim);
        }
    }
    return dataOut;
}

int main(int argc, char** argv) {
    Displayer wp;
    setup_window(&wp);
    sfVector2f position;
    position.x = 100;
    position.y = 100;
    sfText_setPosition(wp.text, position);
    sfText_setColor(wp.text, sfBlue);
    sfText_setCharacterSize(wp.text, 150);
    while (sfRenderWindow_isOpen(wp.window)) {
        // if there is action event then this run
        while (sfRenderWindow_pollEvent(wp.window, &wp.event)) {
            if (wp.event.type == sfEvtClosed) {
                sfRenderWindow_close(wp.window);
            }
        }
        // clear display
        char *data = get_data();
        printf("%s\n", data);
        sfRenderWindow_clear(wp.window, sfWhite);
        sfRenderWindow_drawSprite(wp.window, wp.sprite, NULL);
        sfText_setString(wp.text, "hello world\n");
        sfRenderWindow_drawText(wp.window, wp.text, NULL);

        // display window
        sfRenderWindow_display(wp.window);
        free(data);
        
    }

    sfRenderWindow_destroy(wp.window);
    
}
