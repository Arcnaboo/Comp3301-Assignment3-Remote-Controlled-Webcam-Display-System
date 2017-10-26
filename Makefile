# Add additional project sources like this:
# SRCS += X.c
#
# all the files will be generated with this name (main.o, etc)

#Name of Project

CC = gcc
CFLAGS = -Wall -lpthread -std=gnu99 -lm -lmemcached
DFLAGS = -lcsfml-graphics -lcsfml-window
WFLAGS = -lv4l2

.PHONY: all

all: s4382911_WCDShell s4382911_WCDWebcamControl s4382911_WCDDisplayer s4382911_WCDRemoteControl

s4382911_WCDShell: s4382911_WCDShell.o s4382911_MemcachedLibrary.o
	$(CC) $(CFLAGS) s4382911_WCDShell.o s4382911_MemcachedLibrary.o -o wcdshell
    
s4382911_WCDRemoteControl: s4382911_WCDRemoteControl.o s4382911_MemcachedLibrary.o
	$(CC) $(CFLAGS) s4382911_WCDRemoteControl.o s4382911_MemcachedLibrary.o -o wcdremote
    
s4382911_WCDWebcamControl: s4382911_WCDWebcamControl.o s4382911_MemcachedLibrary.o
	$(CC) $(CFLAGS) $(WFLAGS) s4382911_WCDWebcamControl.o s4382911_MemcachedLibrary.o -o wcdwebcam
    
s4382911_WCDDisplayer: s4382911_WCDDisplayer.o s4382911_MemcachedLibrary.o
	$(CC) $(CFLAGS) $(DFLAGS) s4382911_WCDDisplayer.o s4382911_MemcachedLibrary.o -o wcddisplayer

s4382911_WCDShell.o: s4382911_WCDShell.c s4382911_MemcachedLibrary.o
	$(CC) $(CFLAGS) -c s4382911_WCDShell.c s4382911_MemcachedLibrary.o -o s4382911_WCDShell.o
    
s4382911_WCDRemoteControl.o: s4382911_WCDRemoteControl.c s4382911_MemcachedLibrary.o
	$(CC) $(CFLAGS) -c s4382911_WCDRemoteControl.c s4382911_MemcachedLibrary.o -o s4382911_WCDRemoteControl.o    

s4382911_WCDWebcamControl.o: s4382911_WCDWebcamControl.c s4382911_MemcachedLibrary.o
	$(CC) $(CFLAGS) $(WFLAGS) -c s4382911_WCDWebcamControl.c s4382911_MemcachedLibrary.o -o s4382911_WCDWebcamControl.o

s4382911_WCDDisplayer.o: s4382911_WCDDisplayer.c s4382911_MemcachedLibrary.o
	$(CC) $(CFLAGS) $(DFLAGS) -c s4382911_WCDDisplayer.c s4382911_MemcachedLibrary.o -o s4382911_WCDDisplayer.o
    
s4382911_MemcachedLibrary.o: s4382911_MemcachedLibrary.c
	$(CC) $(CFLAGS) -c s4382911_MemcachedLibrary.c -o s4382911_MemcachedLibrary.o
