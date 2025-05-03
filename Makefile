CC       := gcc
CFLAGS   := -Wall -Wextra -O2 -pthread `pkg-config --cflags cjson`
LDFLAGS  := `pkg-config --libs cjson`
INCDIR   := include
SRCDIR   := src
OBJDIR   := obj
TARGET   := bank_server

SOURCES  := $(wildcard $(SRCDIR)/*.c)
OBJECTS  := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

