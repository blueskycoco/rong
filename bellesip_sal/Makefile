#!/bin/bsh
CROSS_COMPILE=arm-none-linux-gnueabi-
CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar

SOURCES = src/offeranswer.c src/sal_op_call_transfer.c  \
	src/sal_op_presence.c \
	src/sal_address_impl.c  src/sal_op_events.c src/sal_op_publish.c \
	src/sal.c   src/sal_op_impl.c src/sal_op_registration.c \
	src/sal_impl.c src/sal_op_info.c  src/sal_pres.c \
	src/sal_op_call.c   src/sal_op_message.c   src/sal_sdp.c
INCLUDE =
LIB = ../lib/libsal.a

FLAG_COMPILE=-g -c -Wall -DUSE_BELLESIP -I./include -I../include

OBJS = $(SOURCES:.c=.o)
RM = rm

all:$(LIB)

$(LIB):$(OBJS)
	$(AR) -r  $(LIB)  $(OBJS)
	mkdir -p ../include/bellesip_sal
	cp -rf include/* ../include/bellesip_sal

$(OBJS): %.o: %.c $(INCLUDE) 
	mkdir -p objs/src
	$(CC) $(FLAG_COMPILE) $< -o $@
.PHONY:clean   
clean:
	$(RM) -rf objs src/*.o
