#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>

using namespace std;

#define RANGE_LEN	5

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
		BlockMap(int numBlocks, bool value) {
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

		BlockMap(vector<bool> blockMap) {
			int i = 0;
			this->numBlocks = blockMap.size();
			blocks = blockMap;
			while (i < numBlocks && blocks[i] == true) {
				i++;
			}
			currentBlock = i;
		}

		bool hasBlock(int blockNumber) {
			if (blockNumber < numBlocks) {
				return blocks[blockNumber];
			} else {
				abort();
			}
		}

		bool setBlock(int blockNumber) {
			if (blockNumber < numBlocks) {
				blocks[blockNumber] = true;
			} else {
				abort();
			}
		}

		/* Return the next range of blocks to be fetched. */
		bool nextBlockRange(int &start, int &end) {
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
};

/*
 * Every URL has a corresponding File associated with it.
 * At the moment, this only contains the metadata associated with the URL.
 * Need to decide if we store the data in this structure as well OR
 * in another data structure OR keep the data only on disk.
 */
class File {
	string url;
	BlockMap *blockInfo;

	public:
		File(string url) {
			this->url = url;
			blockInfo = new BlockMap(getNumBlocks(url), false);
		}

		File(string url, int numBlocks, bool hasData) {
			this->url = url;
			blockInfo = new BlockMap(numBlocks, hasData);
		}

		File(string url, vector<bool> blocks) {
			this->url = url;
			blockInfo = new BlockMap(blocks);
		}
};
