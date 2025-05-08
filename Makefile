CC       := gcc
CFLAGS   := -Wall -Wextra -O2 -pthread -Iinclude -Ilib/cJSON $(shell mysql_config --cflags)
LDFLAGS  := $(shell mysql_config --libs)
INCDIR   := include
SRCDIR   := src
CJSONDIR := lib/cJSON
OBJDIR   := obj
TARGET   := bank_server

# 앱 소스 + cJSON 소스
SRCS     := $(wildcard $(SRCDIR)/*.c) $(CJSONDIR)/cJSON.c
# obj/*.o 파일 목록
OBJECTS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(wildcard $(SRCDIR)/*.c)) \
            $(OBJDIR)/cJSON.o

.PHONY: all clean

all: $(TARGET)

# 최종 바이너리 링크
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# src/%.c → obj/%.o
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# cJSON.c → obj/cJSON.o
$(OBJDIR)/cJSON.o: $(CJSONDIR)/cJSON.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# obj 디렉터리 생성
$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)
