#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>

using namespace std;

#define RANGE_LEN	5

/*
 * Get video length information from the URL somehow.
 */
int getNumBlocks(string url) {
	return 0;
}

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
	vector<bool> blocks;
	int numBlocks;
	int currentBlock;

	public:
		BlockMap();

		BlockMap(int numBlocks, bool value);

		BlockMap(vector<bool> blockMap);

		bool hasBlock(int blockNumber);

		bool setBlock(int blockNumber);

		bool nextBlockRange(int &start, int &end);
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

	public:
		File(string url);

		File(string url, int numBlocks, bool hasData);

		File(string url, vector<bool> blocks);

		string getURL() const;

		const BlockMap& getBlockInfo() const;

		void updateBlockInfo(const BlockMap& b);
};

class Client {
	string	ip_address;
	int		port;
	int		socketfd;
	string	directory;
	vector<File>	files;
	vector<Client>	peers;

	public:
	Client(string ip, int port, string dir);
	string getIP() const;
	int getFileIdxByURL(const string& url) const;
	void updateFile(const int& file_idx, const BlockMap& b);
};

BlockMap::BlockMap() {
	numBlocks = 0;
	currentBlock = 0;
}

BlockMap::BlockMap(int numBlocks, bool value) {
	this->numBlocks = numBlocks;
	for (int i = 0; i < numBlocks; i++) {
		blocks.push_back(value);
	}
	if (value) {
		currentBlock = numBlocks;
	} else {
		currentBlock = 0;
	}
}

BlockMap::BlockMap(vector<bool> blockMap) {
	int i = 0;
	this->numBlocks = blockMap.size();
	blocks = blockMap;
	while (i < numBlocks && blocks[i] == true) {
		i++;
	}
	currentBlock = i;
}

bool
BlockMap::hasBlock(int blockNumber) {
	if (blockNumber < numBlocks) {
		return blocks[blockNumber];
	} else {
		abort();
	}
}

bool
BlockMap::setBlock(int blockNumber) {
	if (blockNumber < numBlocks) {
		blocks[blockNumber] = true;
	} else {
		abort();
	}
}

/* Return the next range of blocks to be fetched. */
bool
BlockMap::nextBlockRange(int &start, int &end) {
	start = currentBlock;
	end = currentBlock;
	if (start >= numBlocks) {
		/* Nothing more to fetch. */
		return false;
	}
	for (int i = 1; i <= RANGE_LEN && end + i < numBlocks; i++) {
		end++;
	}
	return true;
}

File::File(string url) {
	this->url = url;
	BlockMap tmp(getNumBlocks(url), false);
	blockInfo = tmp;
}

File::File(string url, int numBlocks, bool hasData) {
	this->url = url;
	BlockMap tmp(numBlocks, hasData);
	blockInfo = tmp;
}

File::File(string url, vector<bool> blocks) {
	this->url = url;
	BlockMap tmp(blocks);
	blockInfo = tmp;
}

string
File::getURL() const {
	return url;
}

const BlockMap&
File::getBlockInfo() const {
	return blockInfo;
}

void
File::updateBlockInfo(const BlockMap& b) {
	blockInfo = b;
}

Client::Client(string ip_address, int port, string directory) {
	this->ip_address = ip_address;
	this->port = port;
	this->directory = directory;
	/* Bind to port and store the socketfd */
	/* Initialize list of files and blockmaps */
}

string
Client::getIP() const {
	return ip_address;
}

int
Client::getFileIdxByURL(const string& url) const {
	for (int i = 0; i < files.size(); i++) {
		if (files[i].getURL() == url) {
			return i;
		}
	}
	return -1;
}

void
Client::updateFile(const int& file_idx, const BlockMap& b) {
	files[file_idx].updateBlockInfo(b);
}
