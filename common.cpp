#include "common.h"

/*
 * Get video length information from the URL somehow.
 */
int getNumBlocks(string url) {
	return 0;
}

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

char *
BlockMap::serialize(int &size) const {
	char *data;
	int offset = 0;

	// 2 integers and an array of booleans
	size = sizeof(int) * 2 + numBlocks * sizeof(bool);
	data = new char[size];
	memcpy(data, (char *)&numBlocks, sizeof(int));
	offset += sizeof(int);
	memcpy(data + offset, (char *)&currentBlock, sizeof(int));
	offset += sizeof(int);
	for (int i = 0; i < numBlocks; i++) {
		bool tmp = blocks[i];
		memcpy(data + offset, (char *)&tmp, sizeof(bool));
		offset += sizeof(bool);
	}
	return data;
}

void
BlockMap::deserialize(char *data, const int& size) {
	int nB, cB, offset = 0;

	memcpy((char *)&nB, data, sizeof(int));
	offset += sizeof(int);
	memcpy((char *)&cB, data + offset, sizeof(int));
	offset += sizeof(int);
	numBlocks = nB;
	currentBlock = cB;

	blocks.clear();
	for (int i = 0; i < numBlocks; i++) {
		bool tmp;
		memcpy((char *)&tmp, data + offset, sizeof(bool));
		offset += sizeof(bool);
		blocks.push_back(tmp);
	}

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
