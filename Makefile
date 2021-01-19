TARGET= terminal_monitor
OBJS = terminal_monitor.o

CC = gcc
CFLAGS = -Wall -O2

RM = rm -f

$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) -lpthread
$(OBJS):%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
.PHONY:clean
clean:
	-$(RM) $(TARGET) $(OBJS)
