ARTIFUCT = main
CC = gcc
CSRC = main.c
LDFLAGS = -lpthread


$(ARTIFUCT): $(CSRC)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(ARTIFUCT)