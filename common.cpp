#include "common.h"
#include <algorithm>
#define DEFAULT_BLK_SIZE	1000000

pthread_mutex_t fetch_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

int cache_size;

bool
sendSocketData(int sockfd, int size, char *data) {
	int bytes_pending = size, start_offset = 0;
	while (bytes_pending != 0) {
		int bytes_sent = send(sockfd, data + start_offset, bytes_pending, 0);
		if (bytes_sent == -1) {
			cout<<"send failed: "<<strerror(errno)<<endl;
			return false;
		}
		//cout<<"Sent "<<bytes_sent<<" bytes"<<endl;
		start_offset += bytes_sent;
		bytes_pending -= bytes_sent;
	}
	return true;
}

//Assumes that memory has been allocated for data
bool
recvSocketData(int sockfd, int size, char *data) {
	int bytes_pending = size, start_offset = 0;
	while (bytes_pending != 0) {
		int bytes_rcvd = recv(sockfd, data + start_offset, bytes_pending, 0);
		if (bytes_rcvd == -1) {
			cout<<"recv failed: "<<strerror(errno)<<endl;
			return false;
		}
		//cout<<"Received "<<bytes_rcvd<<" bytes"<<endl;
		bytes_pending -= bytes_rcvd;
		start_offset += bytes_rcvd;
	}
	return true;
}

/*
 * Get video length information from the URL somehow.
 */
int getVideoLength(string url) {
	char command[256];
	int retries = 1;

	sprintf(command, "./youtube_get_video_size.pl https://www.youtube.com/watch?v=%s", url.c_str());
	//cout<<"Command is "<<command<<endl;
	FILE *fp = NULL;
	while (retries > 0) {
		fp = popen(command, "r");
		if (!fp) {
			cout<<"Could not get video size! Retrying."<<endl;
			retries--;
			continue;
		} else {
			break;
		}
	}
	if (!fp) {
		cout<<"Could not execute script!"<<endl;
		exit(1);
	}
	char buffer[20];
	while (!feof(fp)) {
		if (fgets(buffer, 128, fp) == NULL) {
			cout<<"Could not get output from script!"<<endl;
			exit(1);
		}
		//cout<<"video length is "<<buffer<<endl;
		break;
	}
	pclose(fp);
	return atoi(buffer);
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

bool
BlockMap::unsetBlock(int blockNumber) {
	if (blockNumber < numBlocks) {
		blocks[blockNumber] = false;
		if (blockNumber < currentBlock) {
			currentBlock = blockNumber;
		}
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
	BlockMap tmp(getVideoLength(url), false);
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
	this->cache_used = 0;
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
		//cout<<tmp<<endl;
		string filename = tmp;

		memset(tmp, 0, 1024);
		fscanf(fp, "filetype: %s\n", tmp);
		//cout<<tmp<<endl;
		string filetype = tmp;

		int filesize;
		fscanf(fp, "filesize: %d\n", &filesize);
		//cout<<filesize<<endl;

		int blocksize;
		fscanf(fp, "blocksize: %d\n", &blocksize);
		//cout<<blocksize<<endl;
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
			//cout<<blockname<<endl;
			int blocknum = atoi(urlresult->d_name);
			//cout<<"Have block: "<<blocknum<<endl;
			bmap[blocknum] = true;
			if (blocknum != num_blocks - 1) {
				cache_used += blocksize;
			} else {
				cache_used += (filesize % blocksize);
			}
		}
		closedir(urldir);
		File f(filename, bmap, filesize, blocksize);
		files.push_back(f);
	}
	closedir(dir);

	from_source = from_peer = from_cache = 0;
}

/* Called holding exclusive lock (client_mutex) and having locked fetch_mutex */
void
Client::handleCache(string name) {
	cout<<"Cache used at the beginning = "<<cache_used<<endl;
	int idx = getFileIdxByURL(name);
	int begin = time(NULL) % files.size();
	for (int tries = 5; tries > 0; tries--) {
		int i = begin;
		while (1) {
			if (i == idx) {
				i = (i + 1) % files.size();
				if (i == begin) {
					break;
				} else {
					continue;
				}
			}
			BlockMap b = files[i].getBlockInfo();
			int bsize, fsize;
			files[i].getSizeInfo(fsize, bsize);
			int numblocks = fsize / bsize;
			if (fsize % bsize != 0) {
				numblocks++;
			}
			int start, end;
			b.nextBlockRange(start, end);
			start--;
			int numDeleted = 0;
			string fname = files[i].getURL();
			string folder = url_to_folder[fname];
			char tmp[10];
			while (numDeleted < 10 && start >= 0) {
				memset(tmp, 0, sizeof(char) * 10);
				sprintf(tmp, "%d", start);
				string blockname = folder + "/" + tmp;
				if (unlink(blockname.c_str()) == 0) {
					//cout<<"Deleting "<<blockname<<endl;
					b.unsetBlock(start);
				}
				if (start != (numblocks - 1)) {
					cache_used -= bsize;
				} else {
					cache_used -= (fsize % bsize);
				}
				start--;
				numDeleted++;
			}
			files[i].updateBlockInfo(b);
			if (cache_used / (cache_size / 100) < 85) {
				tries = 0;
				break;
			}
			i = (i + 1) % files.size();
			if (i == begin) {
				break;
			}
		}
	}
	cout<<"Cache used at the end = "<<cache_used<<endl;
}

char *
Client::getBlock(string name, int start, int req_size, int& resp_size, int& fsize) {

	//cout<<"name = "<<name<<endl;
	pthread_t pf;
	//pthread_rwlock_rdlock(this->client_mutex);	// Upgrading rd to wr not working. investigate later
	pthread_rwlock_wrlock(this->client_mutex);
	int i = getFileIdxByURL(name);
	if (i == -1) {
		//cout<<"Not found in cache!"<<endl;
		int filesize = getVideoLength(name.c_str());
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
			//cout<<"foldername: "<<foldername<<endl;
			int max_id = atoi(foldername);
			if (folder_id <= max_id) {
				folder_id = max_id + 1;
			}
		}
		//cout<<"FOlder id = "<<folder_id<<endl;
		char foldername[64];
		sprintf(foldername, "%s/%d", directory.c_str(), folder_id);
		//cout<<"foldername = "<<foldername<<endl; 
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
		fprintf(metadata, "filetype: webm\n");
		fprintf(metadata, "filesize: %d\n", filesize);
		fprintf(metadata, "blocksize: %d\n", blocksize);
		fclose(metadata);
		url_to_folder.insert(pair<string, string>(name, foldername));
		File f(name, bmap, filesize, blocksize);
		//pthread_rwlock_wrlock(this->client_mutex);	//upgrade to write lock
		files.push_back(f);
		//pthread_rwlock_unlock(this->client_mutex);	//downgrade to read lock
		i = getFileIdxByURL(name);
	}
	if (i == -1) {
		cout<<"SHould have found file!"<<endl;
		exit(1);
	}
	File f = files[i];
	int filesize, blocksize;
	f.getSizeInfo(filesize, blocksize);
	//cout<<"Blocksize according to file = "<<blocksize<<endl;
	fsize = filesize;
	int blocknum = start / blocksize;
	int blockOffset = start - blocknum * blocksize;
	string folder = url_to_folder[name];
	char tmp[10];
	sprintf(tmp, "%d", blocknum);
	string blockname = folder + "/" + tmp;
	int prefetched = 0;
	pthread_mutex_lock(&fetch_mutex);
	while (std::find(files[i].downloading.begin(), files[i].downloading.end(), blocknum) != files[i].downloading.end()) {
		prefetched = 1;
		//cout<<"Block "<<blocknum<<" already being downloaded"<<endl;
		pthread_mutex_unlock(&fetch_mutex);
		pthread_rwlock_unlock(this->client_mutex);
		sleep(2);
		pthread_rwlock_wrlock(this->client_mutex);
		i = getFileIdxByURL(name);
		pthread_mutex_lock(&fetch_mutex);
	}
	BlockMap b = files[i].getBlockInfo();

	char *resp_data = new char[blocksize];
	if (b.hasBlock(blocknum)) {
		pthread_mutex_unlock(&fetch_mutex);
		//cout<<"Servicing "<<blocknum<<" from cache"<<endl;
		FILE *fp = fopen(blockname.c_str(), "r");
		fseek(fp, blockOffset, 0);
		resp_size = fread(resp_data, 1, blocksize, fp);
		pthread_mutex_lock(&stats_mutex);
		from_cache += (resp_size / 1000);
		pthread_mutex_unlock(&stats_mutex);
		fclose(fp);
		pthread_rwlock_unlock(this->client_mutex);
		//char *tmpname = (char *)name.c_str();
		//pthread_create(&pf, NULL, prefetchBlock, (void *)tmpname);
		return resp_data;
	} else {
		if (prefetched == 1) {
			cout<<"Should have been in cache!"<<endl;
			abort();
		}
		files[i].downloading.push_back(blocknum);
		if (cache_used / (cache_size / 100) > 90) {
			handleCache(name);
		}
		pthread_mutex_unlock(&fetch_mutex);
		/* First check if a peer has the block */
		int peer_id = peerWithBlock(name, blocknum);
		if (peer_id != -1) {
			//cout<<"Peer "<<peer_id<<" has block."<<endl;
			Client peer = peers[peer_id];
			string peer_ip_address = peer.getIP();
			int port = peer.getPort();
			pthread_rwlock_unlock(this->client_mutex);
			int peerfd = connectToHost(peer_ip_address, port);
			if (peerfd == -1) {
				cout<<"Connect to peer failed with error: "<<strerror(errno)<<endl;
				exit(1);
			}

			char header[HEADER_SZ];
			int op = CLIENT_REQ_DATA;
			int req_size = sizeof(int) +	//blocknumber
							sizeof(int) + name.length(); //URL info
			memcpy(header, (char *)&op, sizeof(int));
			memcpy(header + sizeof(int), (char *)&req_size, sizeof(int));
			/*
			int bytes_sent = send(peerfd, header, HEADER_SZ, 0);
			assert(bytes_sent == HEADER_SZ);
			*/
			bool peer_ret;
			char *data;
			int offset;
			peer_ret = sendSocketData(peerfd, HEADER_SZ, header);
			if (peer_ret) {
				data = new char[req_size];
				offset = 0;
				memcpy(data + offset, (char *)&blocknum, sizeof(int));
				offset += sizeof(int);

				int url_len = name.length();
				memcpy(data + offset, (char *)&url_len, sizeof(int));
				offset += sizeof(int);

				memcpy(data + offset, name.c_str(), url_len);
				offset += url_len;

				/*
				   bytes_sent = send(peerfd, data, req_size, 0);
				//cout<<"Bytes sent = "<<bytes_sent<<endl;
				assert(bytes_sent == req_size);
				 */
				peer_ret = sendSocketData(peerfd, req_size, data);
				free(data);	//Only client serialize uses malloc
			}

			char response_header[HEADER_SZ];
			if (peer_ret) {
				//int bytes_rcvd = recv(peerfd, header, HEADER_SZ, 0); 
				//assert(bytes_rcvd == HEADER_SZ);
				peer_ret = recvSocketData(peerfd, HEADER_SZ, header);
			}
			char *recv_data;
				int num_bytes;
			if (peer_ret) {
				//WE dont care about op field in response
				memcpy((char *)&num_bytes, header + sizeof(int), sizeof(int));
				//cout<<"About to receive "<<num_bytes<<" from peer"<<endl;
				recv_data = new char[num_bytes];
				peer_ret = recvSocketData(peerfd, num_bytes, recv_data);
			}
			close(peerfd);

			if (peer_ret) {
				FILE *blockfile = fopen(blockname.c_str(), "w");
				if (!blockfile) {
					cout<<"Could not create block "<<blocknum<<" on disk!"<<endl;
					exit(1);
				}
				fwrite(recv_data, 1, num_bytes, blockfile);
				fclose(blockfile);
				pthread_rwlock_wrlock(this->client_mutex);
				cache_used += num_bytes;
				i = getFileIdxByURL(name);		//i could have changed due to queryTracker
				if (i == -1) {
					cout<<"Client corruption!"<<endl;
					exit(1);
				}
				b = files[i].getBlockInfo();
				if (!b.hasBlock(blocknum)) {
					b.setBlock(blocknum);
					files[i].updateBlockInfo(b);
					//cout<<"For block "<<blocknum<<" increment fetch_source by "<<((endRange - startRange + 1) / 1000)<<endl;
					pthread_mutex_lock(&stats_mutex);
					from_peer += (num_bytes / 1000);
					pthread_mutex_unlock(&stats_mutex);
				}
				//b.setBlock(blocknum);
				//pthread_rwlock_wrlock(this->client_mutex);	//upgrade to write lock
				//files[i].updateBlockInfo(b);
				//pthread_rwlock_unlock(this->client_mutex);	//downgrade to read lock
				resp_size = num_bytes - blockOffset;
				//from_peer += (num_bytes / 1000);
				memcpy(resp_data, recv_data + blockOffset, resp_size);
				close(peerfd);
				delete[] recv_data;
				pthread_rwlock_unlock(this->client_mutex);
				pthread_mutex_lock(&fetch_mutex);
				vector<int>::iterator it = std::find(files[i].downloading.begin(), files[i].downloading.end(), blocknum);
				if (it != files[i].downloading.end()) {
					files[i].downloading.erase(it);
				} else {
					cout<<"Could not remove block "<<blocknum<<" from downloading queue"<<endl;
				}
				pthread_mutex_unlock(&fetch_mutex);
				return resp_data;
			}
		}

		//Fetch from Youtube
		char command[256];
		int startRange= blocknum * blocksize;
		int endRange = (startRange + blocksize >= filesize) ? (filesize - 1) : (startRange + blocksize - 1);
		pthread_rwlock_unlock(this->client_mutex);
		sprintf(command, "./youtube_get_video.pl https://www.youtube.com/watch?v=%s %s %d %d > /dev/null", name.c_str(), blockname.c_str(), startRange, endRange);
		cout<<"Command = "<<command<<endl;
		int retries = 10;

		FILE *fp = NULL;
		while (retries > 0) {
			system(command);
			fp = fopen(blockname.c_str(), "r");
			if (!fp) {
				if (retries == 0) {
					cout<<"Failed to get block from youtube!"<<endl;
					exit(1);
				}
				retries--;
				cout<<"Retrying!"<<endl;
				sleep(1);

			} else {
				break;
			}
		}
		fseek(fp, blockOffset, 0);
		resp_size = fread(resp_data, 1, blocksize, fp);
		fclose(fp);
		pthread_rwlock_wrlock(this->client_mutex);
		cache_used += resp_size;
		i = getFileIdxByURL(name);		//i could have changed due to queryTracker
		if (i == -1) {
			cout<<"Client corruption!"<<endl;
			exit(1);
		}
		b = files[i].getBlockInfo();
		if (!b.hasBlock(blocknum)) {
			b.setBlock(blocknum);
			files[i].updateBlockInfo(b);
			//cout<<"For block "<<blocknum<<" increment fetch_source by "<<((endRange - startRange + 1) / 1000)<<endl;
			pthread_mutex_lock(&stats_mutex);
			from_source += ((endRange - startRange + 1) / 1000);
			pthread_mutex_unlock(&stats_mutex);
		}
		//pthread_rwlock_wrlock(this->client_mutex);	//upgrade to write lock
		//pthread_rwlock_unlock(this->client_mutex);	//downgrade to read lock

		pthread_rwlock_unlock(this->client_mutex);
		pthread_mutex_lock(&fetch_mutex);
		vector<int>::iterator it = std::find(files[i].downloading.begin(), files[i].downloading.end(), blocknum);
		if (it != files[i].downloading.end()) {
			files[i].downloading.erase(it);
		} else {
			cout<<"Could not remove block "<<blocknum<<" from downloading queue"<<endl;
		}
		pthread_mutex_unlock(&fetch_mutex);
		return resp_data;
	}
}

int
Client::getPrefetchOffset(char *name, int offset, int bsize, int fsize) {
	pthread_rwlock_rdlock(this->client_mutex);
	int i = getFileIdxByURL(name);
	if (i == -1) {
		cout<<"Could not find file!"<<endl;
		pthread_rwlock_unlock(this->client_mutex);
		return -1;
	}
	BlockMap b = files[i].getBlockInfo();
	files[i].getSizeInfo(fsize, bsize);
	/*
	int start, end;
	if (!b.nextBlockRange(start, end)) {
		cout<<"No more to prefetch!"<<endl;
		pthread_rwlock_unlock(this->client_mutex);
		return -1;
	}
	*/
	int start = offset / bsize;
	while (start * bsize < fsize) {
		if (b.hasBlock(start) == false) {
			break;
		}
		start++;
	}
	pthread_rwlock_unlock(this->client_mutex);
	if (start * bsize >= fsize) {
		 return -1;
	} else {
		return (start * bsize);
	}
}

void
Client::sendBlock(int sockfd, string name, int blocknum) {
	int offset = 0;
	char header[HEADER_SZ];
	int op, fsize, bsize, resp_size;
	
	pthread_rwlock_rdlock(this->client_mutex);
	int i = getFileIdxByURL(name);
	if (i == -1) {
		cout<<"Tracker corruption!"<<endl;
		exit(1);
	}
	files[i].getSizeInfo(fsize, bsize);
	pthread_rwlock_unlock(this->client_mutex);
	char *data = getBlock(name, blocknum * bsize, bsize, resp_size, fsize);

	memcpy(header + offset, (char *)&op, sizeof(int));
	offset += sizeof(int);
	memcpy(header + offset, (char *)&resp_size, sizeof(int));
	offset += sizeof(int);

	/*
	int bytes_sent = send(sockfd, header, HEADER_SZ, 0);
	assert(bytes_sent == HEADER_SZ);
	*/
	sendSocketData(sockfd, HEADER_SZ, header);
	/*
	int bytes_pending = resp_size, start_offset = 0;
	while (bytes_pending != 0) {
		bytes_sent = send(sockfd, data + start_offset, bytes_pending, 0);
		//cout<<"Sent "<<bytes_sent<<" bytes"<<endl;
		start_offset += bytes_sent;
		bytes_pending -= bytes_sent;
	}
	*/
	sendSocketData(sockfd, resp_size, data);
}

string
Client::getIP() const {
	return ip_address;
}

int
Client::getPort() const {
	return port;
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
	delete[] tmp;

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
	delete[] tmp;

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
		delete[] file_data;
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

void
Client::setTrackerFd(int sockfd) {
	trackerfd = sockfd;
}

void
Client::connectToTracker(string tracker_ip, int port) {
	/*
	struct addrinfo hints, *res;
	int sockfd;
	char portstr[8];
	sprintf(portstr, "%d", port);

	// first, load up address structs with getaddrinfo():

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(tracker_ip.c_str(), (const char *)portstr, &hints, &res);

	// make a socket:

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		cout<<"Could not create socket!"<<endl;
		exit(1);
		abort();
	}
	// connect!

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		cout<<"Could not connect to tracker\n"<<endl;
		exit(1);
		abort();
	}
	*/
	trackerfd = connectToHost(tracker_ip, port);
}

void
Client::registerWithTracker() {
	char header[HEADER_SZ];
	int op = TRACKER_OP_REGISTER;
	char *data;
	int size;
	int sockfd = trackerfd;

	data = serialize(size);
	memcpy(header, (char *)&op, sizeof(int));
	memcpy(header + sizeof(int), (char *)&size, sizeof(int));
	/*
	int bytes_sent = send(sockfd, header, HEADER_SZ, 0);
	assert(bytes_sent == HEADER_SZ);
	*/
	sendSocketData(sockfd, HEADER_SZ, header);
	/*
	bytes_sent = send(sockfd, data, size, 0);
	assert(bytes_sent == size);
	*/
	sendSocketData(sockfd, size, data);
	//cout<<"Bytes sent = "<<bytes_sent<<endl;
	free(data);	//Only client serialize uses malloc
}

void
Client::updateOnTracker() {
	char header[HEADER_SZ];
	int op = TRACKER_OP_UPDATE;
	char *data;
	int size;
	int sockfd = trackerfd;

	//cout<<"UPdate tracker"<<endl;
	data = serialize(size);
	memcpy(header, (char *)&op, sizeof(int));
	memcpy(header + sizeof(int), (char *)&size, sizeof(int));
	/*
	int bytes_sent = send(sockfd, header, HEADER_SZ, 0);
	assert(bytes_sent == HEADER_SZ);
	*/
	sendSocketData(sockfd, HEADER_SZ, header);

	/*
	bytes_sent = send(sockfd, data, size, 0);
	assert(bytes_sent == size);
	*/
	sendSocketData(sockfd, size, data);
	//cout<<"Bytes sent = "<<bytes_sent<<endl;
	free(data);	//Only client serialize uses malloc
}

void
Client::queryTracker() {
	char header[HEADER_SZ];
	int op = TRACKER_OP_QUERY;
	char *data;
	int size = 1;	//packet size doesnt matter for query
	int sockfd = trackerfd;

	//cout<<"Query tracker"<<endl;
	memcpy(header, (char *)&op, sizeof(int));
	memcpy(header + sizeof(int), (char *)&size, sizeof(int));
	/*
	int bytes_sent = send(sockfd, header, HEADER_SZ, 0);
	assert(bytes_sent == HEADER_SZ);
	*/
	sendSocketData(sockfd, HEADER_SZ, header);
	//Now, receive info from tracker

	//int bytes_rcvd = recv(sockfd, header, HEADER_SZ, 0); 
	//assert(bytes_rcvd == HEADER_SZ);
	recvSocketData(sockfd, HEADER_SZ, header);

	//Ignore op field in response and take the packet size
	memcpy((char *)&size, header + sizeof(int), sizeof(int));
	data = new char[size];
	/*
	bytes_rcvd = recv(sockfd, data, size, 0);
	assert(bytes_rcvd == size);
	*/
	recvSocketData(sockfd, size, data);
	//cout<<"bytes_rcvd = "<<bytes_rcvd<<endl;

	int num_clients, offset = 0;
	memcpy((char *)&num_clients, data + offset, sizeof(int));
	offset += sizeof(int);

	peers.clear();
	for (int i = 0; i < num_clients; i++) {
		int csize;
		memcpy((char *)&csize, data + offset, sizeof(int));
		offset += sizeof(int);
		Client c;
		c.deserialize(data + offset, csize);
		offset += csize;
		//c.print();
		if (c.getIP() == ip_address && c.getPort() == port) {
			//cout<<"Self - ignore."<<endl;
		} else {
			peers.push_back(c);
		}
	}
	delete[] data;
}

void
Client::disconnect() {
	char header[HEADER_SZ];
	int op = TRACKER_OP_QUIT;
	char *data;
	int size;	//packet size doesnt matter for disconnect
	int sockfd = trackerfd;

	int len = ip_address.length();
	size = sizeof(int) + len;
	//cout<<"Query tracker"<<endl;
	memcpy(header, (char *)&op, sizeof(int));
	memcpy(header + sizeof(int), (char *)&size, sizeof(int));
	/*
	int bytes_sent = send(sockfd, header, HEADER_SZ, 0);
	assert(bytes_sent == HEADER_SZ);
	*/
	sendSocketData(sockfd, HEADER_SZ, header);

	data = new char[size];
	int offset = 0;
	char *tmp = (char *)ip_address.c_str();
	memcpy(data + offset, (char *)&len, sizeof(int));
	offset += sizeof(int);
	memcpy(data + offset, tmp, len);
	sendSocketData(sockfd, size, data);
	delete[] data;
}

bool
Client::hasFileBlock(const int& file_idx, const int& blocknum) {
	BlockMap b = files[file_idx].getBlockInfo();
	return b.hasBlock(blocknum);
}

int
Client::peerWithBlock(const string& name, const int& blocknum) {
	int i;
	int file_idx;

	for (i = 0; i < peers.size(); i++) {
		if ((file_idx = peers[i].getFileIdxByURL(name)) == -1) {
			continue;
		}
		if (peers[i].hasFileBlock(file_idx, blocknum)) {
			return i;
		}
	}
	return -1;
}

void
Client::printStats() {
	static int ctime = 0;
	cout<<endl;
	cout<<"Statistics at time "<<ctime<<"!"<<endl;
	cout<<"Data dowloaded from Youtube: "<<from_source<<endl;
	cout<<"Data fetched from Peer: "<<from_peer<<endl;
	cout<<"Data serviced from Cache: "<<from_cache<<endl;
	cout<<endl;
	ctime += STATS_SLEEP_TIME;
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

int
connectToHost(const string& ip, const int& port) {
	struct addrinfo hints, *res;
	int sockfd;
	char portstr[8];
	sprintf(portstr, "%d", port);

	// first, load up address structs with getaddrinfo():

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(ip.c_str(), (const char *)portstr, &hints, &res);

	// make a socket:

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		cout<<"Could not create socket!"<<endl;
		exit(1);
		abort();
	}
	// connect!

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		cout<<"Could not connect to host\n"<<endl;
		exit(1);
		abort();
	}
	return sockfd;
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
	//cout<<start<<endl;
}

// return file size in bytes
int
readFile(const char *name) {
	struct stat st;
	stat(name, &st);
	//cout<<"File size = "<<st.st_size<<endl;
	return st.st_size;
}

char *
getFileName(char *header) {
	char *buffer;
	char *name = strstr(header, "GET /");
	//cout<<name<<endl;
	name += strlen("GET /");
	char *tmp = strtok_r(name, " ", &buffer);
	//cout<<tmp<<endl;
	return tmp;
}

