all: tracker client
CFLAGS="-g"
tracker:	common.cpp tracker.cpp
			g++ $(CFLAGS) common.cpp tracker.cpp -o tracker -lpthread

client:		common.cpp client.cpp
			g++ $(CFLAGS) common.cpp client.cpp -o client

clean:
			rm -f tracker client
