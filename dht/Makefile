all: client
CFLAGS="-g"

client:		common.cpp client.cpp parser.c tracker.cpp
			g++ $(CFLAGS) common.cpp client.cpp tracker.cpp parser.c -o client -lpthread -I /usr/include/libxml2 -lxml2

clean:
			rm -f client
