CC       := gcc
CFLAGS   := -Wall -Wextra -O2 -pthread \
            -Iinclude -Ilib/cJSON \
            $(shell mysql_config --cflags) \
            -I$(shell mysql_config --variable=pkgincludedir)/..
LDFLAGS  := $(shell mysql_config --libs)

SRCDIR   := src
CJSONDIR := lib/cJSON
OBJDIR   := obj
TARGET   := bank_server

SRCS     := $(wildcard $(SRCDIR)/*.c) $(CJSONDIR)/cJSON.c
OBJECTS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(wildcard $(SRCDIR)/*.c)) \
            $(OBJDIR)/cJSON.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/cJSON.o: $(CJSONDIR)/cJSON.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

