CC = gcc
CFLAGS = -Wall -std=c11 -g

CALENDARPARSERC = src/CalendarParser.c
CALENDARPARSERH = include/CalendarParser.h
CALENDARO = src/CalendarParser.o
CALENDARSO = bin/libcparse.so
APPBIN = APP/bin

LINKEDLISTC = src/LinkedListAPI.c
LINKEDLISTH = include/LinkedListAPI.h
LISTO = src/LinkedListAPI.o
LIBLIST = bin/libllist.a

INCLUDES = include/

all:
	make list
	make parserso

listso: $(LINKEDLISTC) $(LINKEDLISTH)
	$(CC) $(CFLAGS) $(LINKEDLISTC) -o $(LISTO) -I $(INCLUDES)
	$(CC) $(LISTO) -shared -o $(LISTSO)

list: $(LINKEDLISTC) $(LINKEDLISTH)
	$(CC) $(CFLAGS) -c $(LINKEDLISTC) -o $(LISTO) -I $(INCLUDES)
	ar cr $(LIBLIST) $(LISTO)

parserso: $(LINKEDLISTC) $(LINKEDLISTH) $(CALENDARPARSERC) $(CALENDARPARSERH)
	$(CC) $(CFLAGS) -fPIC -c $(CALENDARPARSERC) -o  $(CALENDARO) -I $(INCLUDES)
	$(CC) $(CFLAGS) $(CALENDARO) -Lbin/ -lllist -shared -o $(CALENDARSO)
	cp $(CALENDARSO) $(APPBIN)

run:
	python3 APP/Main.py
clean:
	rm -f $(CALENDARSO) $(CALENDARO) $(LISTO) $(LIBLIST) $(APPBIN)/libcparse.so
