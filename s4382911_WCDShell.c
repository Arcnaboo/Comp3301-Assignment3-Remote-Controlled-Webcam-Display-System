/*
 * @course: Comp 3301
 * @task: Assignment 3
 * @author: Arda 'ARC' Akgur
 * @file: s4382911_WCDShell.c
 * 
 * libraries used: video4linux,  CSFMLgui, lib memcached, lirc
 * 3rd parth program: avconv
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include "s4382911_MemcachedLibrary.h"



// To move the cursor around and clear Terminal
#define clear_terminal() printf("\033[H\033[J")
#define move_cursor(x,y) printf("\033[%d;%dH", (x), (y))

// Color codes for terminal print
#define NORMAL  "\x1B[0m"
#define RED  "\x1B[31m"
#define GREEN  "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE  "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN  "\x1B[36m"
#define WHITE  "\x1B[37m"

// Thread Function Pointers
void *process_updater(void *masterPointer);
void *waiter(void *pidPointer);
static int *pids;

// Function Dictionary
typedef struct {
    
    void (*val[11]) (MemcServer *master, char* command);
    char** key;
} FunctionMap;

// Function prototypes for the FunctionMap
void process_wcd(MemcServer *master, char* command);
void process_sys(MemcServer *master, char* command);
void process_list(MemcServer *master, char* command);
void process_kill(MemcServer *master, char* command);
void process_sel(MemcServer *master, char* command);
void process_img(MemcServer *master, char* command);
void process_clear(MemcServer *master, char* command);
void process_help(MemcServer *master, char* command);
void process_end(MemcServer *master, char* command);
void process_memcached(MemcServer *master, char* command);
void process_pause(MemcServer *master, char* command);



// Arranges the commands to the FunctionMap
void arrange_commands(FunctionMap *map) {
	
	map->key = malloc(sizeof(char*) * 10);

	for (int i = 0; i < 10; i++) {
        
		map->key[i] = malloc(sizeof(char) * 50);
	}
    
	strcat(map->key[0], "wcd");
	strcat(map->key[1], "sys");
	strcat(map->key[2], "list");
	strcat(map->key[3], "kill");
	strcat(map->key[4], "sel");
	strcat(map->key[5], "img");
	strcat(map->key[6], "clear");
	strcat(map->key[7], "help");
	strcat(map->key[8], "end");
    strcat(map->key[9], "memcached");
    
    
	map->val[0] = process_wcd;
	map->val[1] = process_sys;
	map->val[2] = process_list;
	map->val[3] = process_kill;
	map->val[4] = process_sel;
	map->val[5] = process_img;
	map->val[6] = process_clear;
	map->val[7] = process_help;
	map->val[8] = process_end;
    map->val[9] = process_memcached;
    
}

// Checks the command
// if valid command then returns true
bool check_command(char* command) {
    
	char* check = malloc(sizeof(char) * 1024);
	for (int i = 0; i < strlen(command); i++) {
        
		if (command[i] == ' ' || command[i] =='\n') {
            
			check[i] = '\0';
			break;
		}
		check[i] = command[i];
	}
	if (strcmp(check, "wcd") == 0 || strcmp(check, "sys") == 0 ||
		strcmp(check, "kill") == 0 || strcmp(check, "list") == 0 ||
		strcmp(check, "sel") == 0 || strcmp(check, "img") == 0 ||
		strcmp(check, "clear") == 0 || strcmp(check, "help") == 0 ||
		strcmp(check, "end") == 0 || strcmp(check, "memcached") == 0) {

		free(check);
		return true;
	}
	free(check);
	return false;
	
}

// Processes the given command using FunctionMap
void process_command(MemcServer *master, FunctionMap *map, char* command) {
    
	command[strlen(command) - 1] = '\0';
    char temp[strlen(command)];
    
    for (int i = 0; i < strlen(command); i++) {
        
        if (command[i] == ' ') {
            
            temp[i] = '\0';
            break;
        }
        temp[i] = command[i];
    }
    
	for (int i = 0; i < 11; i++) {
        
		if (strcmp(temp, map->key[i]) == 0) {
            
			map->val[i](master, command);
			break;
		}
        
        if (strcmp(command, map->key[i]) == 0) {
            
			map->val[i](master, command);
			break;
		}
	}
}

// Prints the Default Output
void print_default(void) {
    
    clear_terminal();
    move_cursor(0, 0);
    printf("%sWelcome to COMP3301 A3 WCD User Shell\n", CYAN);
}


// Signal Handler
static void ctrl_c_handler(int sig) {
    
    for (int i = 0; i < 2; i++) {
        kill(pids[i], SIGINT);
        
    }
    printf("Terminating WCD\n");
    exit(0);
}

void run_wcd(void) {
    
    int pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", "./wcdwebcam", NULL);
        _exit(127);
    }
    pids[0] = pid;
    pthread_t waiterThread;
    pthread_create(&waiterThread, NULL, waiter, &pid);
    pthread_detach(waiterThread);
    
}

void run_wcd_remote(void) {
    
    int pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", "./wcdremote", NULL);
        _exit(127);
    }
    pids[1] = pid;
    pthread_t waiterThread;
    pthread_create(&waiterThread, NULL, waiter, &pid);
    pthread_detach(waiterThread);
    
}

// main Loop
int main(int argc, char** argv) {
    pids = malloc(sizeof(int) * 2);
    MemcServer master;
    signal(SIGINT, ctrl_c_handler);
	FunctionMap map;
	arrange_commands(&map);
	
    if (!setup_server(&master) || !set_default_variables(&master)) {
        perror("Error creating memcached server");
        return 1;
    }
    run_wcd();
    run_wcd_remote();
    print_default();
    
    sleep(1);
    char buffer[1024];
    
	while (true) {
        
		printf("%s:^) %s", RED, GREEN);
        fgets(buffer, 1024, stdin);
        if (buffer[0] == '\n') continue;
		if (!check_command(buffer)) {
            
			printf("%sInvalid Command !\n", YELLOW);
			printf("%s%s%sPlease write %s* help * %sfor available commands\n", RED,buffer, CYAN, GREEN, CYAN);
			continue;
		}
        
		process_command(&master, &map, buffer);
	} 
	
	return 0;
}

bool is_id_valid(char cm) {
    return cm == '0' || cm == '1' || cm == '2' || cm == '3';
}


/**
* Creates new wcd display
*/
void process_wcd(MemcServer *master, char* command) {
    
    int x,y,h,w;
    char cmd[10], c;
    // wcd x u h x
    sscanf(command, "%s%c%d%c%d%c%d%c%d", cmd, &c, &x, &c, &y, &c, &h, &c, &w);
    cmd[9] = c;
    printf("%sConfirm Command\n%s%s\n", CYAN, YELLOW, cmd);
    
	message_for_who(master, WCDWEBCAM);
	set_the_message(master, WCD_START);
	set_the_start_info(master, x, y, w, h);
	new_message(master);
    //sleep(1);
 
}

void print_module_info(char *buf) {
    char delim[2] = " ";
    char *token = strtok(buf, delim);
    printf("%sModule: %s%s\n", CYAN, GREEN, token);
    token = strtok(NULL, delim);
    printf("    %sUsing: %s%s bytes\n", MAGENTA, YELLOW, token);
    token = strtok(NULL, delim);
    printf("    %sLoaded Instances: %s%s\n", MAGENTA, YELLOW, token);
    token = strtok(NULL, delim);
    printf("    %sDependencies: %s%s\n", MAGENTA, YELLOW, token);
    token = strtok(NULL, delim);
    printf("    %sCurrent State: %s%s\n", MAGENTA, YELLOW, token);
    token = strtok(NULL, delim);
    printf("    %sMemory offset: %s%s$\n", MAGENTA, YELLOW, token);
}

/**
* Relates to task 5
*/
void process_sys(MemcServer *master, char* command) {
    
    FILE *pip;
    if (!strcmp(command, "sys")) {
        pip = popen("cat /proc/modules | grep lirc", "r");
    } else if (!strcmp(command, "sys video") || !strcmp(command, "sys v4l2")) {
        pip = popen("cat /proc/modules | grep v4l2", "r");
    } else {
        return;
    }
    
    
    char buffer[1024];
    while (fgets(buffer, 1024, pip)) {
        buffer[strlen(buffer) - 1] = '\0';
        print_module_info((char*)&buffer[0]);
    }
    fclose(pip);
}

/**
* Lists all alive wcd displays
*/
void process_list(MemcServer *master, char* command) {
    
    message_for_who(master, WCDDISPLAY);
    set_the_message(master, WCD_LIST);
    new_message(master);
	sleep(1);
}

/**
* To kill a display 
*/
void process_kill(MemcServer *master, char* command) {
    // kill 9
    
    printf("$%s$\n", command);
    if (strlen(command) < 6 || !is_id_valid(command[5])) {
        printf("%sInvalid WcdId!", YELLOW);
    }
    message_for_who(master, WCDBOTH);
    set_the_message(master, WCD_KILL);
    select_wcd(master, command[5]);
    new_message(master);
    sleep(1);
	
}

/**
* Select a wcd display 
*/
void process_sel(MemcServer *master, char* command) {
    // sel wcdid
    printf("$%s$\n", command);
    if (strlen(command) < 5 || !is_id_valid(command[4])) {
        printf("%sInvalid WcdId!", YELLOW);
    }
    message_for_who(master, WCDWEBCAM);
    set_the_message(master, WCD_SELECT);
    select_wcd(master, command[4]);
    new_message(master);
    sleep(1);
}

ImgType is_img_valid(char *cm) {
    char *ptr;
    ptr = &cm[4];
    if (!strcmp("RAW", ptr) || !strcmp("raw", ptr)) {
        return IMGRAW;
    }
    if (!strcmp("FLP", ptr) || !strcmp("flp", ptr)) {
        return IMGFLP;
    }
    if (!strcmp("BLR", ptr) || !strcmp("blr", ptr)) {
        return IMGBLR;
    }
    if (cm[4] == 'S' || !strcmp("s", ptr)) {
        return IMGSTART;
    }
    
    if (!strncmp("DLY", ptr, 3) || !strncmp("dly", ptr, 3)) {
        return IMGDLY;
    }
    
    if (!strcmp("ARC", ptr) || !strcmp("arc", ptr)) {
        return IMGARC;
    }
    return IMGINVALID;
}

/**
* Manipulate the display
*/
void process_img(MemcServer *master, char* command) {

    ImgType type = is_img_valid(command);
    if (type == IMGINVALID) {
        printf("%sInvalid Img command!\n", CYAN);
        return;
    }
    
    message_for_who(master, WCDWEBCAM);
    set_the_message(master, WCD_IMG);
    set_img_message(master, type);
    
    if (type == IMGDLY) {
        // img DLY 12412
        int delay;
        sscanf(command, "img DLY %d", &delay);
        if (delay < 0) {
            printf("%sinvalid delay value\n", MAGENTA);
            return;
        }
        set_delay_rate(master, delay);
    }
    
    new_message(master);
    sleep(1);
    
}


/**
* Clear img manipulation
*/
void process_clear(MemcServer *master, char* command) {
    // clear 0
    if (strlen(command) < 7 || !is_id_valid(command[6])) {
        printf("%sInvalid WcdId!", YELLOW);
    }
    
    message_for_who(master, WCDWEBCAM);
    set_the_message(master, WCD_CLEAR);
    select_wcd(master, command[6]);
    new_message(master);
    
    sleep(1);

}

/**
* Prints help commands 
*/
void process_help(MemcServer *master, char* command) {
    
	printf("%sCOMP3301 WCD SYSTEM HELP\n", CYAN);
	printf("%sCommand %sParam %sDescr\n", GREEN, YELLOW, WHITE);
	printf("%s======= %s======%s ======\n", GREEN, YELLOW, WHITE);
	printf("%swcd %s<x> <y> %s<width> <height>", GREEN, YELLOW, MAGENTA);
	printf("%s Create a WCD display at the %s(x, y) %scoordinates, with a certain %s(width/height (pixel))\n", WHITE, YELLOW, WHITE ,MAGENTA);
	printf("%ssys %s=====", GREEN, YELLOW); 
    printf("%s Display stuff.\n", WHITE);
	printf("%slist %s=====", GREEN, YELLOW);
	printf("%s Show the WCD ID values and locations.\n", WHITE);    
    printf("%sclear %s<WCD ID>", GREEN, YELLOW);
	printf("%s Clear img of display with id WCD ID(img manipulation)\n", WHITE);
	printf("%skill %s<WCD ID>", GREEN, YELLOW);
	printf("%s Removed the WCD display with id WCD ID.\n", WHITE);
	printf("%ssel %s<WCD ID>", GREEN, YELLOW);
	printf("%s Select the WCD display with id WCD ID.\n", WHITE);
	printf("%simg %s<COMMAND>", GREEN, YELLOW);
	printf("%s Manipulate the selected WCD display - use '%ssel%s' for select\n", WHITE, GREEN, WHITE);
    printf("\t%sCOMMAND LIST:\n", CYAN);
    printf("\t%sS %s====> %sStart capturing Webcam and display it\n", GREEN, YELLOW, WHITE);
    printf("\t%sRAW %s====> %sDisplay raw image\n", GREEN, YELLOW, WHITE);
    printf("\t%sFLP %s====> %sFlipped (transformed) – (Remote enter – FLP (to 180 degrees)\n", GREEN, YELLOW, WHITE);
    printf("\t%sDLY %s<rate> %sDelayed (downsampled) (Remote enter – DLY(specify rate)\n", GREEN, YELLOW, WHITE);
    printf("\t%sBLR %s====> %sBlur image\n", GREEN, YELLOW, WHITE);
	printf("%shelp %s=====", GREEN, YELLOW);
	printf("%s Displays list and summary of each command.\n", WHITE);
    printf("%smemcached %s=====", GREEN, YELLOW);
	printf("%s Shows all the keys stored by this program to memcached and current values.\n", WHITE);
	printf("%send %s=====", GREEN, YELLOW);
	printf("%s End all processes and threads associated with the WCD.\n", WHITE);
}

/**
* Ends WCD system 
*/
void process_end(MemcServer *master, char* command) {
	exit(0);
}

// processes memcached command 
void process_memcached(MemcServer *master, char* command) {
    MemcValue v[10];
    char **ak = getKeys();
    for (int i = 0; i < 10; i++) { 
        v[i] = m_get_value(master, ak[i]);
        printf("%sMemcached - Key: %s%s %s> Value: %s%s\n", GREEN, CYAN, ak[i], GREEN, YELLOW, v[i].retrievedValue);
    }
    free(ak);
}

// Process waiter thread function
void *waiter(void *pidPointer) {
    
    int *pids = (int *)pidPointer;
    int pid = *pids;
    int got_pid, status;

	/* Wait until child process has finished */
	while ((got_pid = wait(&status))) {   /* go to sleep until something happens */

		/* wait woke up */
		if (got_pid == pid) {
            
            break;	/* this is what we were looking for */
        }
      
		if ((got_pid == -1) && (errno != EINTR)) {
            
			/* an error other than an interrupted system call */
			perror("waitpid");
			exit(30);
		}

	}

	if (WIFEXITED(status))	/* process exited normally */
		printf("WCD process exited with value %d\n", WEXITSTATUS(status));

	else if (WIFSIGNALED(status))	/* child exited on a signal */
		printf("WCD process exited due to signal %d\n", WTERMSIG(status));

	else if (WIFSTOPPED(status))	/* child was stopped */
		printf("WCD process was stopped by signal %d\n", WIFSTOPPED(status));

    return NULL;
}