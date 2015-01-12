#!/bin/bsh
CROSS_COMPILE=arm-none-linux-gnueabi-
CC=$(CROSS_COMPILE)g++
AR=$(CROSS_COMPILE)ar
TARGET=sipvg

SOURCES = \
	src/audio_stream.c  src/call_session.c  src/mschannel.c \
	src/test_audio.c src/callbacks.c   src/channelsys.c \
	src/SipVoiceService.c src/main.c
INCLUDE =
LIB = /home/rdp/sipvg/lib/libvg.a

FLAG_COMPILE=-g -Wall -DUSE_BELLESIP -I./include -I/home/rdp/sipvg/include -I/home/rdp/sipvg/include/bellesip_sal

OBJS = $(SOURCES:.c=.o)
LIB_OBJS = \
	audio_stream.o  call_session.o \
	mschannel.o test_audio.o callbacks.o \
	channelsys.o SipVoiceService.o
MAIN_OBJS = main.o

RM = rm
LDFLAGS=-L/home/rdp/sipvg/lib -lortp -lmediastreamer_base -lmediastreamer_voip -lspeex -lspeexdsp -lbellesip -lantlr3c -lasound

all:$(LIB) $(TARGET)

$(LIB):$(OBJS)
	$(AR) -r $(LIB)  $(LIB_OBJS)
	mkdir -p /home/rdp/sipvg/include/voicegw
	cp -rf include/* /home/rdp/sipvg/include/voicegw
$(TARGET):$(LIB) $(MAIN_OBJS)
	$(CC) -o $@ $(MAIN_OBJS) $(FLAG_COMPILE) $(LDFLAGS) $(LIB) /home/rdp/sipvg/lib/libsal.a
$(OBJS): %.o: %.c $(INCLUDE) 
	mkdir -p objs/src
	$(CC) -c $(FLAG_COMPILE) $< 
.PHONY:clean   
clean:
	rm -rf  *.o sipvg
