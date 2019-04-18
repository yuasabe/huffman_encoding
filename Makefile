CC = gcc
CFLAGS = -Wall -g
OBJS = main.o encode.o

huffman: $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

.PHONY: tmpclean clean
tmpclean:
	rm -f *~
clean: tmpclean
	rm -f $(OBJS) huffman
