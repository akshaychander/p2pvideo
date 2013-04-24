#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstdio>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <map>
#include <stddef.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include "assert.h"

using namespace std;

#define RANGE_LEN	5
#define BACKLOG		20
#define HEADER_SZ	2 * sizeof(int)

enum {
	TRACKER_OP_REGISTER = 0,
	TRACKER_OP_QUERY,
	TRACKER_OP_UPDATE,
	TRACKER_OP_QUIT,
	TRACKER_OP_KEEP_ALIVE,
	CLIENT_REQ_DATA
};

#define STATS_SLEEP_TIME	15
#define FIRST_NODE	-10

/*
 * Each File will have a corresponding BlockMap associated with it.
 * BlockMap is basically a collection of key-value pairs, with the block number
 * being the key and value being either true or false.
 * The value is true if the client has already downloaded this particular block.
 *
 * Defining this as a class now (although we are just using a vector of boolean
 * values gives us the flexibility of changing the implementation later very
 * easily.
 */
class BlockMap {
	int numBlocks;
	int currentBlock;
	vector<bool> blocks;

	public:
		BlockMap();

		BlockMap(int numBlocks, bool value);

		BlockMap(vector<bool> blockMap);

		bool hasBlock(int blockNumber);

		bool setBlock(int blockNumber);

		bool unsetBlock(int blockNumber);

		bool nextBlockRange(int& start, int& end);

		char *serialize(int& size) const;

		void deserialize(char *data, const int& size);

		void print();
};

/*
 * Every URL has a corresponding File associated with it.
 * At the moment, this only contains the metadata associated with the URL.
 * Need to decide if we store the data in this structure as well OR
 * in another data structure OR keep the data only on disk.
 */
class File {
	string url;
	BlockMap blockInfo;
	int filesize;
	int blocksize;

	// NO need to serialize the below

	public:
		vector<int> downloading;

		File();

		File(string url);

		File(string url, int numBlocks, bool hasData);

		File(string url, vector<bool> blocks);

		File(string url, vector<bool> blocks, int filesize, int blocksize);

		string getURL() const;

		BlockMap getBlockInfo() const;

		void getSizeInfo(int& fsize, int& bsize);

		void updateBlockInfo(const BlockMap& b);

		char *serialize(int& size) const;

		void deserialize(char *data, const int& size);

		void print();
};

class Client {
	string	ip_address;
	int		port;
	int		socketfd;
	string	directory;
	vector<File>	files;
	vector<Client>	peers;
	map<string, string> url_to_folder;

	/* Stuff not need to serialize/deserialize */
	int		trackerfd;
	unsigned long long from_source;
	unsigned long long from_cache;
	unsigned long long from_peer;
	int cache_used;

	public:
		pthread_rwlock_t *client_mutex;

		Client();

		Client(string ip, int port, string dir);

		string getIP() const;

		int getPort() const;

		void initialize();

		int getFileIdxByURL(const string& url) const;

		void updateFile(const int& file_idx, const BlockMap& b);

		char *serialize(int& size) const;

		void deserialize(const char *data, const int& size);

		void print();

		void addFile(File f); //for debugging

		void handleCache(string name);

		char* getBlock(string name, int start, int req_size, int& resp_size, int& fsize);

		int getPrefetchOffset(char *name, int offset, int bsize, int fsize);

		//friend void * prefetchBlock(char* name);
		void sendBlock(int sockfd, string name, int blocknum);

		void setTrackerFd(int sockfd);

		void connectToTracker(string tracker_ip, int port);

		void registerWithTracker();

		void updateOnTracker();

		void queryTracker();

		void disconnect();

		bool hasFileBlock(const int& file_idx, const int& blocknum);

		int peerWithBlock(const string& name, const int& blocknum);

		void printStats();
};

class Tracker {
	vector<Client> p2pnodes;
	int port;

	public:
		Tracker(int port);
		int getClientIdxByIP(const string& target);
		bool client_register(const Client& clnt);
		bool update(int idx, const File& file);
		bool update(Client& clnt);
		bool update(Client& clnt, const File& file);
		void deleteClient(string ip);
		BlockMap query(const File& file);
		char *serialize(int& size);
		void deserialize(const char *data, const int& size);
		void print();
};

/* Bind to given port and return socket fd */
int bindToPort(const string& ip, const int& port);
int connectToHost(const string& ip, const int& port);
void getRangeOffset(char *header, int& start, int& end);
int readFile(const char *name);
char *getFileName(char *header);
//void* prefetchBlock(char *name);
bool sendSocketData(int sockfd, int size, char *data);
bool recvSocketData(int sockfd, int size, char *data);
