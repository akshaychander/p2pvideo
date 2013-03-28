all: tracker client

tracker:	common.cpp tracker.cpp
			g++ -g common.cpp tracker.cpp -o tracker -lpthread

client:		common.cpp client.cpp
			g++ -g common.cpp client.cpp -o client

clean:
			rm -f tracker client
