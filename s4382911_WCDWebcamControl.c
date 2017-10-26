/*Comp3301 Assignment 3
** WCDWebcamControl.c
** gcc s4382911_WCDWebcamControl.c  -lv4l2 -lpthread -lcsfml-graphics -lcsfml-window
** Prepared by Arda Akgur
** libraries used: video4linux,  CSFMLgui, lib memcached
** 3rd parth program: avconv
** Video0 read code based on V4L2 video picture grabber by Mauro Carvalho 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>
#include "s4382911_MemcachedLibrary.h"

#define dev_name  "/dev/video0"
#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define HEIGHT 480
#define WIDTH 640

#define MAX_WCD 4

pthread_mutex_t mainmutex = PTHREAD_MUTEX_INITIALIZER;


static int *pids;
bool availableIndexes[4];
bool startCapture[4];

sem_t synch[4];

bool runAllThread = true;

int selected = -1;

typedef struct {
    
    bool running;
    FILE *avconvRead; /*Read from avconv process */
    FILE *avconvWrite; /*Write to Avconv process*/
    FILE *writeFile; /*Write to GUI DIsplayer process */
} AvconvHandler;

typedef struct {
	int index;
    char id;
    bool run; 
    int writePipe[2];
    FILE *writeFile; /* Write to GUI Displayer process */
    ImgType type; /*RAW, FLP, BLR, DLY*/
    int delayrate;
    int internalPipe[2];
    FILE *internalWrite; /* Camrecorder writes to this   */
    FILE *internalRead; /* Reads the frames from camCorder */
    AvconvHandler ah;
    
} WCDDisplayer;

typedef struct {
	
	struct v4l2_format              fmt;
	struct v4l2_buffer              buf;
	struct v4l2_requestbuffers      req;
	enum v4l2_buf_type              type;
	fd_set                          fds;
	struct timeval                  tv;
	int                             r, fd;
	unsigned int                    i, n_buffers;
	struct buffer                   *buffers;
    
    WCDDisplayer **wcds;
    int wcdAmount;
    //WebcamData wd;
    MemcServer ms;
} Master;


void swap_two_wcd(Master *master, int first, int second) {
    
    startCapture[first] = false;
    startCapture[second] = false;
    master->wcds[first]->index = second;
    master->wcds[second]->index = first;
    WCDDisplayer* temp = master->wcds[first];
    master->wcds[first] = master->wcds[second];
    master->wcds[second] = temp;
    startCapture[first] = true;
    startCapture[second] = true;
    
}

void initiate_swap_wcd(Master *master, int index, MoveDirection direction) {
    
    if (master->wcdAmount < 2) return;
    
    
    
    if (direction == M_LEFT) {
        if (index == 0) {
            for (int i = 3; i > 0; i--) {
                if (!availableIndexes[i]) {
                    swap_two_wcd(master, index, i);
                    return;
                }
            }
        } else {
            swap_two_wcd(master, index, index - 1);
            return;
        }
    } else if (direction == M_RIGHT) {
        if (index == 3) {
            for (int i = 0; i < 3; i++) {
                if (!availableIndexes[i]) {
                    swap_two_wcd(master, index, i);
                    return;
                }
            }
        } else {
            swap_two_wcd(master, index, index + 1);
            return;
        }
    }
}

struct buffer {
        void   *start;
        size_t length;
};

static void xioctl(int fh, int request, void *arg) {
        int r;
        do {
                r = v4l2_ioctl(fh, request, arg);
        } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

        if (r == -1) {
                fprintf(stderr, "Error %d, %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}

/* Opens the Webcam Device
**
*/
void openDevice(Master *master) {
    master->fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (master->fd < 0) {
            perror("Cannot open Capture device");
            exit(EXIT_FAILURE);
    }
}

/*
**Sets the format of the video capture device
** RGB24 640x480
*/
void setFormat(Master *master) {
    CLEAR(master->fmt);
    master->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    master->fmt.fmt.pix.width = WIDTH;
    master->fmt.fmt.pix.height = HEIGHT;
    master->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    master->fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    xioctl(master->fd, VIDIOC_S_FMT, &master->fmt);
    if (master->fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
            printf("Libv4l didn't accept RGB24 format. Can't proceed.\\n");
            exit(EXIT_FAILURE);
    }
    if ((master->fmt.fmt.pix.width != WIDTH) || (master->fmt.fmt.pix.height != HEIGHT))
            printf("Warning: driver is sending image at %dx%d\\n",
                    master->fmt.fmt.pix.width, master->fmt.fmt.pix.height);
}

/*
** Initializes Device memory map
*/
void initMmap(Master *master) {
    CLEAR(master->req);
    master->req.count = 2;
    master->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    master->req.memory = V4L2_MEMORY_MMAP;
    xioctl(master->fd, VIDIOC_REQBUFS, &master->req);
    master->buffers = calloc(master->req.count, sizeof(*master->buffers));
    
    for (master->n_buffers = 0; master->n_buffers < master->req.count; ++master->n_buffers) {
        
        CLEAR(master->buf);
        master->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        master->buf.memory = V4L2_MEMORY_MMAP;
        master->buf.index = master->n_buffers;

        xioctl(master->fd, VIDIOC_QUERYBUF, &master->buf);

        master->buffers[master->n_buffers].length = master->buf.length;
        master->buffers[master->n_buffers].start = v4l2_mmap(NULL, master->buf.length,
                    PROT_READ | PROT_WRITE, MAP_SHARED,
                    master->fd, master->buf.m.offset);

        if (MAP_FAILED == master->buffers[master->n_buffers].start) {
                perror("mmap initalization");
                exit(EXIT_FAILURE);
        }
    }
}

/*
** Initializes Query buffer that holds the frames 
*/
void initQueryBuffer(Master *master) {
    
    for (unsigned int i = 0; i < master->n_buffers; ++i) {
        
        CLEAR(master->buf);
        master->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        master->buf.memory = V4L2_MEMORY_MMAP;
        master->buf.index = i;
        xioctl(master->fd, VIDIOC_QBUF, &master->buf);
    }
}

/*
** Shuts down the given AvconvHandler 
** stops the filestream and ends the child process
*/
int shut_off_avconv(AvconvHandler *ah) {
    int fdes, pid;
    sigset_t omask, nmask;
    union wait pstat;
    
    fdes = fileno(ah->avconvRead);
    (void) fclose(ah->avconvWrite);
    (void) fclose(ah->avconvRead);
    
    sigemptyset(&nmask);
 
    sigaddset(&nmask, SIGINT);
    sigaddset(&nmask, SIGQUIT);
    sigaddset(&nmask, SIGHUP);
    (void) sigprocmask(SIG_BLOCK, &nmask, &omask);
    do {
        pid = waitpid(pids[fdes], (int *) &pstat, 0);
    } while (pid == -1 && errno == EINTR);
    (void) sigprocmask(SIG_SETMASK, &omask, NULL);
    pids[fdes] = 0;
    return (pid == -1 ? -1 : pstat.w_status);
}

/* run_avconv()
** Runs an avconv command
** Command needs to be input and output pipe 
** Example 'avconv -f rawvideo -i pipe:0 -f image2pipe pipe:1"
** AvconvHandler pointer as argument stores the read and write file to 
** created pipes
*/
void run_avconv(char *command, AvconvHandler *ah) {
    
    int writepipe[2], readpipe[2], pid, fds;
    // if first time we run an avconv
    if (pids == NULL) {
        
        if ((fds = getdtablesize()) <= 0) return;
        
        if ((pids = (int *)malloc((u_int)(fds * sizeof(int)))) == NULL) return;
          bzero((char *)pids, fds * sizeof(int));
    }
    // create pipes
    pipe(writepipe);
    pipe(readpipe);
    
    pid = fork();
    if (pid == -1){ // error forking
    
        perror("fork for avconv");
        exit(21);
    }
    if (pid == 0) { // child process 
        
        int devNull = open("/dev/null", 2, "w");
        dup2(devNull, 2);
        
        if (writepipe[0] != fileno(stdin)) {
            // close read end dup stdin
            (void) dup2(writepipe[0], fileno(stdin));
            (void) close(writepipe[0]);
        }
        if (readpipe[1] != fileno(stdout)) {
            // close write end dup stdout
            (void) dup2(readpipe[1], fileno(stdout));
            (void) close(readpipe[1]);
        }
        (void) close(writepipe[1]);
        (void) close(readpipe[0]);
        // exec the command 
        execl("/bin/sh", "sh", "-c", command, NULL);
        perror("execcing for avconv");
        _exit(127);
    }
    // parent process, open outRead and outWrite files
    ah->avconvRead = fdopen(readpipe[0], "r");
    ah->avconvWrite = fdopen(writepipe[1], "w");
    (void) close(readpipe[1]);
    (void) close(writepipe[0]);
    // update pids array
    pids[fileno(ah->avconvRead)] = pid;
}

// Shuts down the given WCDDisplayer
void shut_down_wcd(WCDDisplayer *wcd) {
    if (wcd->ah.running) {
        shut_off_avconv(&wcd->ah);
    }
    startCapture[wcd->index] = false;
    fflush(wcd->internalRead);
    fflush(wcd->internalWrite);
    
    wcd->run = false;
    
    fclose(wcd->internalRead);
    fclose(wcd->internalWrite);
    close(wcd->internalPipe[0]);
    close(wcd->internalPipe[1]);
    availableIndexes[wcd->index] = true;
}

// helper to read frames from avconv and write to gui
void *wcd_manipulator_helper(void *arg) {
    WCDDisplayer *wcd = (WCDDisplayer*) arg;
    
    
    
    unsigned char frame[HEIGHT][WIDTH][3];
    while (wcd->run) {
        if (!startCapture[wcd->index]) continue;
        if (wcd->type != IMGARC && wcd->type != IMGDLY && wcd->type != IMGFLP && wcd->type != IMGBLR) continue; //&& wcd->type != IMGDLY && wcd->type != IMGRAW
        
        if (sem_wait(&synch[wcd->index]) == 0) {
            
            fflush(wcd->ah.avconvRead);
            fread(frame, HEIGHT*WIDTH*3, 1, wcd->ah.avconvRead);
            fwrite(frame, HEIGHT*WIDTH*3, 1, wcd->writeFile);
            fflush(wcd->writeFile);
        }
    }
    return NULL;
}

// main image manipulator thread
// for each active wcd 
void *wcd_manipulator(void *arg) {
    
    WCDDisplayer *wcd = (WCDDisplayer*) arg;
    printf("Wcd manip started %c\n", wcd->id);
    
    wcd->ah.running = false;
    if (sem_init(&synch[wcd->index], 0, 0) == -1) {
        perror("sem_init error wcd manipulator thread canceling action");
        shut_down_wcd(wcd);
        
    }
    
    
    pthread_t wcdManipHelper;
    if (pthread_create(&wcdManipHelper, NULL, wcd_manipulator_helper, wcd)) {
        fprintf(stderr, "Error creating wcd manip helper %c\n", wcd->id);
        shut_down_wcd(wcd);
    }
    ImgType current = IMGINVALID;
    unsigned char frame[HEIGHT][WIDTH][3];
    char command[1024];
    
    while (wcd->run) {
        if (!startCapture[wcd->index]) continue;
        if (current != wcd->type) {
            current = wcd->type;
            if (wcd->ah.running) { // if type changed and avconv is runnign
                shut_off_avconv(&wcd->ah);
                wcd->ah.running = false;
            }
            switch (current) {
                case IMGINVALID:
                case IMGSTART:
                    
                case IMGRAW://  overlay=x=main_w-overlay_w-10:y=main_h-overlay_h-10
                    
                    break;
                case IMGFLP:  //
                    sprintf(command, "avconv -f rawvideo -pix_fmt rgb24 -s 640x480 -i pipe:0 -f rawvideo -pix_fmt rgb24 -s 640x480 -vf transpose=1,transpose=1 pipe:1");
                    wcd->ah.running = true;
                    break;
                case IMGBLR:
                    sprintf(command, "avconv -f rawvideo -pix_fmt rgb24 -s 640x480 -i pipe:0 -f rawvideo -pix_fmt rgb24 -s 640x480 -vf \"unsharp=7:7:-2:7:7:-2\" pipe:1");
                    wcd->ah.running = true;
                    break;
                case IMGDLY:
                    sprintf(command, "avconv -f rawvideo -pix_fmt rgb24 -s 640x480 -i pipe:0 -i arcsmall.png -i comp.png -filter_complex 'overlay,overlay=x=250:y=0' -f rawvideo -pix_fmt rgb24 -s 640x480 -vf \"hflip\" pipe:1");
                    wcd->ah.running = true;
                    break;
                case IMGARC:
                    sprintf(command, "avconv -f rawvideo -pix_fmt rgb24 -s 640x480 -i pipe:0 -i arcsmall.png -i arcsmall.png -i arcsmall.png -i arcsmall.png -i arcsmall.png -filter_complex 'overlay,overlay=x=250:y=0,overlay=x=0:y=250,overlay=x=100:y=150,overlay=x=150:y=100' -f rawvideo -pix_fmt rgb24 -s 640x480 -vf \"hflip\" pipe:1");
                    wcd->ah.running = true;
                    break;
            }
            if (wcd->ah.running) run_avconv(command, &wcd->ah);    
        }
        fflush(wcd->internalRead);
        fread(frame, HEIGHT*WIDTH*3, 1, wcd->internalRead);

        if (current == IMGRAW) { // if raw give raw frames 
            fwrite(frame, HEIGHT*WIDTH*3, 1, wcd->writeFile);
            fflush(wcd->writeFile);          
        } else { // else send frames to avconv 
            fwrite(frame, HEIGHT*WIDTH*3, 1, wcd->ah.avconvWrite);
            fflush(wcd->ah.avconvWrite);
            if (sem_post(&synch[wcd->index]) == -1) {
                perror("failed to synchronise wcd");
                shut_down_wcd(wcd);
            }
        }
        
    }
    
    return NULL;
}

// Starts a new WCD display
void start_wcd_display(Master *master, WcdStartInfo *wsi) {
	//printf("starting new wcd dispaly\n");
    WCDDisplayer *res = (WCDDisplayer*)malloc(sizeof(WCDDisplayer));
    
    
    int index;
    for (index = 0; index < MAX_WCD; index++) {
        if (availableIndexes[index]) {
            availableIndexes[index] = false;
            break;
        }
    }
    res->id = '0' + index;
    
    //res.id = '0' + res.index;
    pipe(res->writePipe);
    pid_t pid = fork();
    if (pid == -1){ // error forking
    
        perror("fork error when trying to run display");
        exit(21);
    }
    
    if (pid == 0) {
        //runAllThread = false;
        char command[150]; 
        
        sprintf(command, "./wcddisplayer %c %d %d %d %d", res->id, wsi->x, 
                        wsi->y, wsi->w, wsi->h);
        if (res->writePipe[0] != fileno(stdin)) {
            // close read end dup stdin
            (void) dup2(res->writePipe[0], fileno(stdin));
            (void) close(res->writePipe[0]);
        }
        (void) close(res->writePipe[1]);
        execl("/bin/sh", "sh", "-c", command, NULL);
        _exit(127);
    }
    
    
    res->writeFile = fdopen(res->writePipe[1], "w");
    
    (void) close(res->writePipe[0]);

    res->type = IMGRAW;
    pipe(res->internalPipe);
    res->internalWrite = fdopen(res->internalPipe[1], "w");
    res->internalRead = fdopen(res->internalPipe[0], "r");
    res->run = true;
    master->wcds[index] = res;
    master->wcds[index]->index = index;
    master->wcdAmount++;
    pthread_t imageManip;
    pthread_create(&imageManip, NULL, wcd_manipulator, res);
    pthread_detach(imageManip);


}

// WEBCAM thread, camptures live frames from webcam
void *camRecorder(void *mp) {
    Master *master = (Master*)mp;
    printf("Camrecorder Started\n");
    master->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(master->fd, VIDIOC_STREAMON, &master->type);
    while (true) {
        do {
            FD_ZERO(&master->fds);
            FD_SET(master->fd, &master->fds);

            /* Timeout. */
            master->tv.tv_sec = 2;
            master->tv.tv_usec = 0;

            master->r = select(master->fd + 1, &master->fds, NULL, NULL, &master->tv);
        } while ((master->r == -1 && (errno = EINTR)));
        
        if (master->r == -1) {
                perror("error selecting frame");
                exit(errno);
        }

        CLEAR(master->buf);
        master->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        master->buf.memory = V4L2_MEMORY_MMAP;
        xioctl(master->fd, VIDIOC_DQBUF, &master->buf);
        for (int wx = 0; wx < MAX_WCD; wx++) {
            if (startCapture[wx]) {
                //pthread_mutex_lock(&mainmutex);
                fwrite(master->buffers[master->buf.index].start, 
                        master->buf.bytesused, 1, master->wcds[wx]->internalWrite);
                fflush(master->wcds[wx]->internalWrite);
                //pthread_mutex_unlock(&mainmutex);
            }
        }
        
        xioctl(master->fd, VIDIOC_QBUF, &master->buf);
    }
    return NULL;
}

// exit protocol
void eit_protocol(Master *master) {
    
    for (int i = 0; i < MAX_WCD; i++) {
        if (!availableIndexes[i]) {
            shut_down_wcd(master->wcds[i]);
        }
    }
    
    master->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(master->fd, VIDIOC_STREAMOFF, &master->type);
    int i;
    for (i = 0; i < master->n_buffers; ++i) {
        v4l2_munmap(master->buffers[i].start, master->buffers[i].length);
    }
    v4l2_close(master->fd);
}

Master *mpp;

// signal handler
void sighandlerwcd(int sig) {
    
    eit_protocol(mpp);
    printf("Terminating Webcam Control\n");
    exit(0);
}

// prepares webcam 
void prepareDevice(Master *master) {
    
   
    // start webcam driver
    openDevice(master);
    // set video format 
    setFormat(master);
    // init memory map
    initMmap(master);
    // init query buffer
    initQueryBuffer(master);
    // start capturing thread 
    
}


// processes img manipulation request 
void process_img_request(Master *master, ImgType type) {
    
    if (type == IMGINVALID) return;
    
    if (type == IMGSTART) {
        if (availableIndexes[selected]) {
            printf("WCD %d has not been initialized yet, please use - help\n", selected);
            return;
        }
        startCapture[selected] = true;
        printf("Starting capturing WCD-%d\n", selected);
        return;
    }
    if (type == IMGDLY) {
        master->wcds[selected]->delayrate = receive_delay_rate(&master->ms);
    }
    master->wcds[selected]->type = type;
    
}

// ./webcam standalone
int main(int argc, char **argv) {
        
        if (argc > 2) return 1;
    
        signal(SIGINT, sighandlerwcd);
        signal(SIGKILL, sighandlerwcd);
        signal(SIGPIPE, sighandlerwcd);
        signal(SIGSEGV, sighandlerwcd);
        Master *master = (Master*)malloc(sizeof(Master));
        mpp = master;

        
        for (int i = 0; i < MAX_WCD; i++) {
        availableIndexes[i] = true;
        startCapture[i] = false;
        }
    
        master->wcds = (WCDDisplayer**)malloc(sizeof(WCDDisplayer*) * MAX_WCD);
        master->wcdAmount = 0;

        if (!setup_server(&master->ms)) {
            perror("Setting up MemcServer WCD");
            return 2;
        }
        prepareDevice(master);
        pthread_t capturer;
        pthread_create(&capturer, NULL, camRecorder, master);
        pthread_detach(capturer);
        if (argc == 2) {
            WcdStartInfo sinfo;
            sinfo.x = 0;
            sinfo.y = 0;
            sinfo.w = 1024;
            sinfo.y = 748;
            sinfo.id = 0;
            start_wcd_display(master, &sinfo);
        }
        
        // chill out and wait 
        //shell or remote commands via memcached
        while (1) {
            if (is_there_new_message(&master->ms)) {
				char *who = who_received_message(&master->ms);
				if (strcmp(WCDWEBCAM, who) && strcmp(WCDBOTH, who)) continue;
				
				MessageType type = decode_wcd_message(&master->ms);
				selected = get_selected(&master->ms);
				if (type == WCDSTART) {
					WcdStartInfo *wsis = get_start_info(&master->ms);
					start_wcd_display(master, wsis);
					//master->wcds[wcd->index] = wcd;
					//startCapture[wcd->index] = true;;
				}
                
                if (type == WCDSELECT) {
                    if (selected == -1 || selected > 3) {
                        printf("Please select wcd id between 0 and 3\n");
                        no_message(&master->ms);
                        continue;
                    }
                    if (availableIndexes[selected]) {
                        printf("WCD %d has not been initialized yet, please use - help\n", selected);
                        selected = -1;
                        no_message(&master->ms);
                        continue;
                    }
                }
                
                if (type == WCDIMG) {
                    if (selected == -1) {
                        printf("Please first select or create WCD\n");
                        no_message(&master->ms);
                        continue;
                    } else if (selected > 3) {
                        printf("Selected WCD has not been initialized yet, please use help\n");
                        selected = -1;
                        no_message(&master->ms);
                        continue;
                    }
                    ImgType type = get_img_type(&master->ms);
                    process_img_request(master, type);
                }
                
                if (type == WCDCLEAR) {
                    
                    if (selected == -1) {
                        printf("Please first select or create WCD\n");
                        no_message(&master->ms);
                        continue;
                    }
                    printf("Clearing image stream to wcd %d\n", selected);
                    master->wcds[selected]->type = IMGRAW;
                    startCapture[selected] = false;
                }
                
                if (type == WCDKILL) {
                    if (selected == -1 || selected > 3) {
                        printf("Please first select or create WCD\n");
                        no_message(&master->ms);
                        continue;
                    }
                    if (availableIndexes[selected]) {
                        printf("Selected WCD has not been initialized yet, please use help\n");
                        selected = -1;
                        no_message(&master->ms);
                        continue;
                    }
                    printf("Shutting down wcd %d\n", selected);
                    shut_down_wcd(master->wcds[selected]);
                }
                
                if (type == KILLALL) {
                    for (int i = 0; i < MAX_WCD; i++) {
                        if (!availableIndexes[i]) {
                            printf("Shutting down wcd %d\n", i);
                            shut_down_wcd(master->wcds[i]);
                        }
                    }
                }
                
                if (type == WCDMOVE) {
                    
                }
				no_message(&master->ms);
			}
        }
        
        eit_protocol(master);
        return 0;
}
