######################################################################
#  Makefile for guci
#  
#  Dov Grobgeld
#  Mon May 17 13:38:11 2004
######################################################################

CFLAGS = -g -Wall `pkg-config --cflags gtk+-2.0`
LIBS   = `pkg-config --libs gtk+-2.0`

SRC = guci.c unicode_names.c
OBJ = $(SRC:.c=.o)

guci : $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIBS)

dist:
	tar -czf guci-0.0.2.tar.gz $(SRC) generate_unicode_names Makefile README COPYING ChangeLog

unicode_names.c: generate_unicode_names
	./generate_unicode_names UnicodeData-3.0.txt > unicode_names.c
