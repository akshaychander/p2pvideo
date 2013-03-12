#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>

using namespace std;

#define RANGE_LEN	5

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
		File();

		File(string url);

		File(string url, int numBlocks, bool hasData);

		File(string url, vector<bool> blocks);

		string getURL() const;

		const BlockMap& getBlockInfo() const;

		void updateBlockInfo(const BlockMap& b);

		char *serialize(int& size) const;

		void deserialize(char *data, const int& size);
};

class Client {
	string	ip_address;
	int		port;
	int		socketfd;
	string	directory;
	vector<File>	files;
	vector<Client>	peers;

	public:
		Client();

		Client(string ip, int port, string dir);

		string getIP() const;

		int getFileIdxByURL(const string& url) const;

		void updateFile(const int& file_idx, const BlockMap& b);

		char *serialize(int& size) const;

		void deserialize(const char *data, const int& size);
};

