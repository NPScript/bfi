default_target: all

all:
	g++ bfi.cpp -o bfi -lncurses

clean:
	rm bfi

install: all
	cp bfi /usr/local/bin/

uninstall:
	rm /usr/local/bin/bfi
