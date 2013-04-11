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
	TRACKER_OP_KEEP_ALIVE,
	CLIENT_REQ_DATA
};

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

	public:
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

		char* getBlock(string name, int start, int req_size, int& resp_size, int& fsize);

		//friend void * prefetchBlock(char* name);
		void sendBlock(int sockfd, string name, int blocknum);

		void setTrackerFd(int sockfd);

		void connectToTracker(string tracker_ip, int port);

		void registerWithTracker();

		void updateOnTracker();

		void queryTracker();

		bool hasFileBlock(const int& file_idx, const int& blocknum);

		int peerWithBlock(const string& name, const int& blocknum);
};

/* Bind to given port and return socket fd */
int bindToPort(const string& ip, const int& port);
int connectToHost(const string& ip, const int& port);
void getRangeOffset(char *header, int& start, int& end);
int readFile(const char *name);
char *getFileName(char *header);
//void* prefetchBlock(char *name);
