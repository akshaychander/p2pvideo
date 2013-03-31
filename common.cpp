#include "common.h"
#define DEFAULT_BLK_SIZE	1000000
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
		if (currentBlock == blockNumber) {
			while (currentBlock < numBlocks && blocks[currentBlock] == true) {
				currentBlock++;
			}
		}
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

void BlockMap::print() {
	cout<<"numBlocks: "<<numBlocks<<endl;
	cout<<"currentBlock: "<<currentBlock<<endl;
	for (int i = 0; i < numBlocks; i++) {
		if (blocks[i] == true)
			cout<<"Block "<<i<<": True"<<endl;
		else
			cout<<"Block "<<i<<": False"<<endl;
	}
	cout<<endl;
}

File::File() {
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

File::File(string url, vector<bool> blocks, int filesize, int blocksize) {
	this->url = url;
	BlockMap tmp(blocks);
	blockInfo = tmp;
	this->filesize = filesize;
	this->blocksize = blocksize;
}

string
File::getURL() const {
	return url;
}

BlockMap
File::getBlockInfo() const {
	return blockInfo;
}

void
File::getSizeInfo(int& fsize, int& bsize) {
	fsize = filesize;
	bsize = blocksize;
}

void
File::updateBlockInfo(const BlockMap& b) {
	blockInfo = b;
}

char *
File::serialize(int &size) const {
	char *data;
	int offset = 0, block_data_size;

	char *blockdata = blockInfo.serialize(block_data_size);
	int url_size = url.length();
	/*
	 * Int to store size of the string, followed by actual string,
	 * then filesize and blocksize
	 * and then the size of the BlockMap data followed by the
	 * BlockMap data.
	 */
	size = sizeof(int) + url_size + 3 * sizeof(int) + block_data_size;

	data = new char[size];
	memcpy(data, (char *)&url_size, sizeof(int));
	offset += sizeof(int);

	const char *tmp = url.c_str();
	memcpy(data + offset, tmp, url_size);
	offset += url_size;

	memcpy(data + offset, (char *)&filesize, sizeof(int));
	offset += sizeof(int);

	memcpy(data + offset, (char *)&blocksize, sizeof(int));
	offset += sizeof(int);

	memcpy(data + offset, (char *)&block_data_size, sizeof(int));
	offset += sizeof(int);
	memcpy(data + offset, blockdata, block_data_size);

	// No longer need the packed blockdata.
	delete[] blockdata;
	return data;
}

void
File::deserialize(char *data, const int& size) {
	int offset = 0;
	char *tmp;
	int	url_len;
	int block_data_size;
	
	memcpy((char *)&url_len, data, sizeof(int));
	offset += sizeof(int);

	tmp = new char[url_len + 1];
	memcpy(tmp, data + offset, url_len);
	offset += url_len;
	tmp[url_len] = '\0';
	url = tmp;
	delete[] tmp;

	memcpy((char *)&filesize, data + offset, sizeof(int));
	offset += sizeof(int);

	memcpy((char *)&blocksize, data + offset, sizeof(int));
	offset += sizeof(int);
	
	memcpy((char *)&block_data_size, data + offset, sizeof(int));
	offset += sizeof(int);

	blockInfo.deserialize(data + offset, block_data_size);
}

void File::print() {
	cout<<"URL: "<<url<<endl;
	blockInfo.print();
	cout<<endl;
}

Client::Client() {
}

Client::Client(string ip_address, int port, string directory) {
	this->ip_address = ip_address;
	this->port = port;
	this->directory = directory;
	this->socketfd = -1;	//for debugging
	initialize();
	/* Bind to port and store the socketfd */
	/* Initialize list of files and blockmaps */
}


void
Client::initialize() {
	DIR	*dir;
	struct dirent *buf;

	dir = opendir(directory.c_str());
	if (!dir) {
		cout<<"Initialize failed!"<<endl;
		exit(1);
	}

	struct dirent* pentry = (struct dirent *)malloc(offsetof(struct dirent, d_name) + 
							pathconf(directory.c_str(), _PC_NAME_MAX) + 1);

	while (1) {
		struct dirent* result;
		readdir_r(dir, pentry, &result);
		if(!result) {
			break;
		}
		string subname = result->d_name;
		if (subname == "." || subname == "..") {
			continue;
		}
		//cout<<"name: "<<subname<<endl;
		string fullpath = directory + "/" + subname + "/metadata";
		FILE *fp = fopen(fullpath.c_str(), "r");
		if (!fp) {
			cout<<"Invalid metadata format for "<<subname<<"!"<<endl;
			continue;
		}

		char tmp[1024];
		fscanf(fp, "url: %s\n", tmp);
		cout<<tmp<<endl;
		string filename = tmp;

		memset(tmp, 0, 1024);
		fscanf(fp, "filetype: %s\n", tmp);
		cout<<tmp<<endl;
		string filetype = tmp;

		int filesize;
		fscanf(fp, "filesize: %d\n", &filesize);
		cout<<filesize<<endl;

		int blocksize;
		fscanf(fp, "blocksize: %d\n", &blocksize);
		cout<<blocksize<<endl;
		fclose(fp);
		string urlpath = directory + "/" + subname;
		url_to_folder.insert(pair<string, string>(filename, urlpath));

		vector<bool> bmap;
		int num_blocks = filesize / blocksize;
		if (filesize % blocksize != 0) {
			num_blocks++;
		}
		bmap.assign(num_blocks, false);
		DIR *urldir = opendir(urlpath.c_str());
		struct dirent* urlpentry = (struct dirent *)malloc(offsetof(struct dirent, d_name) + 
				pathconf(urlpath.c_str(), _PC_NAME_MAX) + 1);
		while (1) {
			struct dirent* urlresult;
			readdir_r(urldir, urlpentry, &urlresult);
			if(!urlresult) {
				break;
			}
			string blockname = urlresult->d_name;
			if (blockname == "." || blockname == ".." || blockname == "metadata") {
				continue;
			}
			cout<<blockname<<endl;
			int blocknum = atoi(urlresult->d_name);
			cout<<"Have block: "<<blocknum<<endl;
			bmap[blocknum] = true;
		}
		closedir(urldir);
		File f(filename, bmap, filesize, blocksize);
		files.push_back(f);
	}
	closedir(dir);
}

char *
Client::getBlock(string name, int start, int req_size, int& resp_size, int& fsize) {
	cout<<"name = "<<name<<endl;
	int i = getFileIdxByURL(name);
	if (i == -1) {
		cout<<"Not found in cache. Fetch from source"<<endl;
		
		/* Replace this with fetch from youtube */
		int filesize = readFile(name.c_str());
		int blocksize = DEFAULT_BLK_SIZE;
		int num_blocks = filesize / blocksize;
		if (filesize % blocksize != 0) {
			num_blocks++;
		}
		vector<bool>bmap;
		bmap.assign(num_blocks, false);
		map<string, string>::iterator it;
		int folder_id = 1;
		for (it = url_to_folder.begin(); it != url_to_folder.end(); it++) {
			const char *tmp = (it->second).c_str();
			const char *foldername = strrchr(tmp, '/') + 1;
			cout<<"foldername: "<<foldername<<endl;
			int max_id = atoi(foldername);
			if (folder_id <= max_id) {
				folder_id = max_id + 1;
			}
		}
		cout<<"FOlder id = "<<folder_id<<endl;
		char foldername[64];
		sprintf(foldername, "%s/%d", directory.c_str(), folder_id);
		cout<<"foldername = "<<foldername<<endl; 
		if (mkdir(foldername ,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH) != 0) {
			cout<<"mkdir failed with error: "<<strerror(errno)<<endl;
			exit(1);
		}
		string filename = foldername;
		filename += "/metadata";
		FILE *metadata = fopen(filename.c_str(), "w");
		if (!metadata) {
			cout<<"metadata creation failed!"<<endl;
			exit(1);
		}
		fprintf(metadata, "url: %s\n", name.c_str());
		fprintf(metadata, "filetype: mp4\n");
		fprintf(metadata, "filesize: %d\n", filesize);
		fprintf(metadata, "blocksize: %d\n", blocksize);
		fclose(metadata);
		url_to_folder.insert(pair<string, string>(name, foldername));
		File f(name, bmap, filesize, blocksize);
		files.push_back(f);
		i = getFileIdxByURL(name);
	}
	if (i == -1) {
		cout<<"SHould have found file!"<<endl;
		exit(1);
	}
	File f = files[i];
	int filesize, blocksize;
	f.getSizeInfo(filesize, blocksize);
	cout<<"Blocksize according to file = "<<blocksize<<endl;
	fsize = filesize;
	int blocknum = start / blocksize;
	BlockMap b = f.getBlockInfo();
	int blockOffset = start - blocknum * blocksize;
	string folder = url_to_folder[name];
	char tmp[10];
	sprintf(tmp, "%d", blocknum);
	string blockname = folder + "/" + tmp;
	char *resp_data = new char[blocksize];
	if (b.hasBlock(blocknum)) {
		cout<<"Servicing "<<blocknum<<" from cache"<<endl;
		FILE *fp = fopen(blockname.c_str(), "r");
		fseek(fp, blockOffset, 0);
		resp_size = fread(resp_data, 1, blocksize, fp);
		fclose(fp);
		return resp_data;
	} else {
		FILE *source = fopen(name.c_str(), "r");
		if (!source) {
			cout<<"Could not fetch from source!"<<endl;
			exit(1);
		}
		int range_offset = blocknum * blocksize;
		if (range_offset > 0) {
			fseek(source, range_offset, 0);
		}
		char block_data[blocksize];
		int bsize = fread(block_data, 1, blocksize, source);
		fclose(source);
		FILE *blockfile = fopen(blockname.c_str(), "w");
		if (!blockfile) {
			cout<<"Could not create block "<<blocknum<<" on disk!"<<endl;
			exit(1);
		}
		fwrite(block_data, 1, bsize, blockfile);
		fclose(blockfile);
		b.setBlock(blocknum);
		files[i].updateBlockInfo(b);
		resp_size = bsize - blockOffset;
		memcpy(resp_data, block_data + blockOffset, resp_size);
		return resp_data;
	}
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

/*
 * Not including peer information at the moment.
 * Need to revisit.
 */
char *
Client::serialize(int& size) const {
	int offset = 0, string_length, num_files;
	char *data, *file_data;
	const char* tmp;

	size = sizeof(int) + ip_address.length() +	//Length of URL and the URL
			2 * sizeof(int) +					//port and socketfd
			sizeof(int) + directory.length() +	//Length of directory and directory
			sizeof(int);						//Number of files


	data = (char *)malloc(sizeof(char) * size);

	string_length = ip_address.length();
	memcpy(data, (char *)&string_length, sizeof(int));
	offset += sizeof(int);

	tmp = ip_address.c_str();
	memcpy(data + offset, tmp, string_length);
	offset += string_length;

	memcpy(data + offset, (char *)&port, sizeof(int));
	offset += sizeof(int);

	// Probably unnecessary but why not?
	memcpy(data + offset, (char *)&socketfd, sizeof(int));
	offset += sizeof(int);

	string_length = directory.length();
	memcpy(data + offset, (char *)&string_length, sizeof(int));
	offset += sizeof(int);

	tmp = directory.c_str();
	memcpy(data + offset, tmp, string_length);
	offset += string_length;

	num_files = files.size();
	memcpy(data + offset, (char *)&num_files, sizeof(int));
	offset += sizeof(int);

	for (int i = 0; i < files.size(); i++) {
		int file_size;
		file_data = files[i].serialize(file_size);
		if (!file_data) {
			cout<<"File "<<i<<" could not be serialized!"<<endl;
			continue;
		}
		size += sizeof(int) + file_size;	//File length and the File
		data = (char *)realloc(data, size);
		if (!data) {
			cout<<"Client::serialize Memory allocation failure!"<<endl;
			abort();
			return NULL;
		}
		memcpy(data + offset, (char *)&file_size, sizeof(int));
		offset += sizeof(int);

		memcpy(data + offset, file_data, file_size);
		offset += file_size;
		delete[] file_data;
	}
	return data;
}

void
Client::deserialize(const char *data, const int& size) {
	int offset = 0, num_files, file_size, tmp_num;
	char *tmp, *file_data;

	memcpy((char *)&tmp_num, data, sizeof(int));
	offset += sizeof(int);

	tmp = new char[tmp_num + 1];
	memcpy(tmp, data + offset, tmp_num);
	offset += tmp_num;
	tmp[tmp_num] = '\0';
	ip_address = tmp;
	free(tmp);

	memcpy((char *)&tmp_num, data + offset, sizeof(int));
	offset += sizeof(int);
	port = tmp_num;

	memcpy((char *)&tmp_num, data + offset, sizeof(int));
	offset += sizeof(int);
	socketfd = tmp_num;

	memcpy((char *)&tmp_num, data + offset, sizeof(int));
	offset += sizeof(int);

	tmp = new char[tmp_num + 1];
	memcpy(tmp, data + offset, tmp_num);
	offset += tmp_num;
	tmp[tmp_num] = '\0';
	directory = tmp;
	free(tmp);

	memcpy((char *)&num_files, data + offset, sizeof(int));
	offset += sizeof(int);
	files.clear();
	for (int i = 0; i < num_files; i++) {
		File f;
		memcpy((char *)&file_size, data + offset, sizeof(int));
		offset += sizeof(int);

		file_data = new char[file_size];
		memcpy(file_data, data + offset, file_size);
		offset += file_size;

		f.deserialize(file_data, file_size);
		files.push_back(f);
		free(file_data);
	}
}

void Client::print() {
	cout<<"ip_address: "<<ip_address<<endl;
	cout<<"port: "<<port<<endl;
	cout<<"socketfd: "<<socketfd<<endl;
	cout<<"directory: "<<directory<<endl;
	for (int i = 0; i < files.size(); i++) {
		cout<<"File "<<i<<endl;
		files[i].print();
	}
	cout<<endl;
}

void Client::addFile(File f) {
	files.push_back(f);
}

int
bindToPort(const string& ip, const int& port) {
	int sockfd;
	struct addrinfo hints, *s_addr;
	char service[10];

	sprintf(service, "%d", port); 
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip.c_str(), service, &hints, &s_addr);
	sockfd = socket(s_addr->ai_family, s_addr->ai_socktype, s_addr->ai_protocol);
	if (sockfd == -1) {
		return -1;
	}
	int yes = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		cout<<"Reusesocket failed"<<endl;
		exit(1);
	}
	if (bind(sockfd, s_addr->ai_addr, s_addr->ai_addrlen) == 0) {
		return sockfd;
	} else {
		return -1;
	}
}

void
getRangeOffset(char *header, int& start, int& end) {
	char *buffer;
	char *range = strstr(header, "Range: bytes=");
	//cout<<"range = "<<range<<endl;
	range += strlen("Range: bytes=");
	char *tmp = strtok_r(range, "-", &buffer);
	//cout<<tmp<<endl;
	start = atoi(tmp);
	cout<<start<<endl;
}

// return file size in bytes
int
readFile(const char *name) {
	struct stat st;
	stat(name, &st);
	cout<<"File size = "<<st.st_size<<endl;
	return st.st_size;
}

char *
getFileName(char *header) {
	char *buffer;
	char *name = strstr(header, "GET /");
	//cout<<name<<endl;
	name += strlen("GET /");
	char *tmp = strtok_r(name, " ", &buffer);
	cout<<tmp<<endl;
	return tmp;
}
