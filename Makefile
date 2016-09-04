# Makefile for ratmenu
#
# Jonathan Walther
# djw@reactor-core.org

PROG   = ratmenu

DESTDIR=
PREFIX=$(DESTDIR)/usr
MANDIR=$(PREFIX)/man

OPTIMIZE ?= -O2
DEBUG    ?= 
WARN     ?= -Wall -pedantic

LIBS   = -L/usr/X11R6/lib -lX11
CFLAGS += $(OPTIMIZE) $(WARN) $(DEBUG)

$(PROG): $(PROG).c
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LIBS) $(LDFLAGS) -o $@

clean:
	rm -f $(PROG)

# TODO: make real tests out of these by running them under Xnest/Xephyr and
#   feeding them keystrokes, and checking their output values...
humantest: $(PROG)
	-./$(PROG) -align right foo "echo foo" bar "echo bar" menuitemsuberalle "./$(PROG) x x y y z z" exit "echo exit"
	-./$(PROG) foofoofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofofooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo "echo foo" bar "echo bar"
	-./$(PROG) f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff fa aa aa aa aa aa
	-./$(PROG) -persist -io 5 -style dreary 1 "echo 1 bar" 2 "echo 2 bar" 3 "echo 3 bar" 4 "echo 4 bar" 5 "echo 5 bar" 6 "echo 6 bar" 7 "echo 7 bar" 8 "echo 8 bar" 9 "echo 9 bar" 10 "echo 10 bar" 11 "echo 11 bar" 12 "echo 12 bar" 13 "echo 13 bar" 14 "echo 14 bar" 15 "echo 15 bar" 16 "echo 16 bar" 17 "echo 17 bar" 18 "echo 18 bar" 19 "echo 19 bar" 20 "echo 20 bar" 21 "echo 21 bar" 22 "echo 22 bar" 23 "echo 23 bar" 24 "echo 24 bar" 25 "echo 25 bar" 26 "echo 26 bar" 27 "echo 27 bar" 28 "echo 28 bar" 29 "echo 29 bar" 30 "echo 30 bar" 31 "echo 31 bar" 32 "echo 32 bar" 33 "echo 33 bar" 34 "echo 34 bar" 35 "echo 35 bar" 36 "echo 36 bar" 37 "echo 37 bar" 38 "echo 38 bar" 39 "echo 39 bar" 40 "echo 40 bar" 41 "echo 41 bar" 42 "echo 42 bar" 43 "echo 43 bar" 44 "echo 44 bar" 45 "echo 45 bar" 46 "echo 46 bar" 47 "echo 47 bar" 48 "echo 48 bar" 49 "echo 49 bar" 50 "echo 50 bar"
	-./$(PROG) -persist -io 5 -style snazzy 1 "echo 1 bar" 2 "echo 2 bar" 3 "echo 3 bar" 4 "echo 4 bar" 5 "echo 5 bar" 6 "echo 6 bar" 7 "echo 7 bar" 8 "echo 8 bar" 9 "echo 9 bar" 10 "echo 10 bar" 11 "echo 11 bar" 12 "echo 12 bar" 13 "echo 13 bar" 14 "echo 14 bar" 15 "echo 15 bar" 16 "echo 16 bar" 17 "echo 17 bar" 18 "echo 18 bar" 19 "echo 19 bar" 20 "echo 20 bar" 21 "echo 21 bar" 22 "echo 22 bar" 23 "echo 23 bar" 24 "echo 24 bar" 25 "echo 25 bar" 26 "echo 26 bar" 27 "echo 27 bar" 28 "echo 28 bar" 29 "echo 29 bar" 30 "echo 30 bar" 31 "echo 31 bar" 32 "echo 32 bar" 33 "echo 33 bar" 34 "echo 34 bar" 35 "echo 35 bar" 36 "echo 36 bar" 37 "echo 37 bar" 38 "echo 38 bar" 39 "echo 39 bar" 40 "echo 40 bar" 41 "echo 41 bar" 42 "echo 42 bar" 43 "echo 43 bar" 44 "echo 44 bar" 45 "echo 45 bar" 46 "echo 46 bar" 47 "echo 47 bar" 48 "echo 48 bar" 49 "echo 49 bar" 50 "echo 50 bar"

dist: clean
	P=$$(basename $$PWD); cd ..; tar czf $$P.tar.gz $$P

doc:
	groff -Tascii -man ratmenu.1|less

install: $(PROG)
	install -D -p -m 755 -s $(PROG) $(PREFIX)/bin/$(PROG)
# ends up in *both* /usr/man and /usr/share/man, just give up here
#	install -D -p -m 755 $(PROG).1 $(MANDIR)/man1/$(PROG).1
