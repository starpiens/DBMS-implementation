.SUFFIXES: .c .o

CC=gcc
CFLAGS+= -g -std=c11 -Wall -O2 -fPIC -I $(INCDIR)

SRCDIR=src/
INCDIR=include/
LIBDIR=lib/

# SRCS:=$(wildcard src/*.c)
# OBJS:=$(SRCS:.c=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.c
TARGET_OBJ:=$(SRCDIR)main.o

TARGET=main

# Include more files if you write another source file.
SRCS_FOR_LIB:=$(SRCDIR)bpt.c $(SRCDIR)file.c
OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.c=.o)

all: $(TARGET)

$(TARGET): $(TARGET_OBJ)
	$(CC) $(CFLAGS) -o $(SRCDIR)bpt.o -c $(SRCDIR)bpt.c
	$(CC) $(CFLAGS) -o $(SRCDIR)file.o -c $(SRCDIR)file.c
	make static_library
	$(CC) $(CFLAGS) -o $@ $^ -L $(LIBDIR) -lbpt

clean:
	rm $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBDIR)*

library:
	gcc -shared -Wl,-soname,libbpt.so -o $(LIBDIR)libbpt.so $(OBJS_FOR_LIB)

static_library:
	ar cr $(LIBDIR)libbpt.a $(OBJS_FOR_LIB)
