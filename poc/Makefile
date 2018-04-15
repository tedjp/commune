COMPILE = gcc -Wall -Wextra

all: listener sender

sender: sender.c
	$(COMPILE) -o $@ $^

listener: listener.c
	$(COMPILE) -o $@ $^

clean:
	rm -f listener sender
