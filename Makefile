all: tracker client
CFLAGS="-g"
tracker:	common.cpp tracker.cpp
			g++ $(CFLAGS) common.cpp tracker.cpp -o tracker -lpthread

client:		common.cpp client.cpp parser.c
			g++ $(CFLAGS) common.cpp client.cpp parser.c -o client -lpthread -I /usr/include/libxml2 -lxml2

clean:
			rm -f tracker client
