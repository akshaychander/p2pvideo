#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <map>
#include <stddef.h>
#include <dirent.h>
#include <unistd.h>
#include "common.h"

using namespace std;
int streaming_port = 10000;
/* Todo: Handle SIGPIPE */
/*
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
		cout<<"reuse failed"<<endl;
		exit(1);
	}
	if (bind(sockfd, s_addr->ai_addr, s_addr->ai_addrlen) == 0) {
		return sockfd;
	} else {
		return -1;
	}
}
*/

void
getRangeOffset(char *header, int& start, int& end) {
	char *range = strstr(header, "Range: bytes=");
	cout<<range<<endl;
	range += strlen("Range: bytes=");
	char *tmp = strtok(range, "-");
	cout<<tmp<<endl;
	start = atoi(tmp);
	cout<<start<<endl;
}

// return file size in bytes
long readFile(char *name) {
	struct stat st;
	stat(name, &st);
	cout<<"File size = "<<st.st_size<<endl;
	return st.st_size;
}

map<string, string> url_to_folder;
vector<File> filelist;

char *
getBlock(string name, int start, int req_size, int& resp_size, int& fsize) {
	int i;
	cout<<"name = "<<name<<endl;
	for (i = 0; i < filelist.size(); i++) {
		cout<<filelist[i].getURL()<<endl;
		if (filelist[i].getURL() == name) {
			break;
		}
	}
	if (i == filelist.size()) {
		cout<<"Not found in cache. Fetch from source"<<endl;

		/* Replace this with fetch from youtube */
		/*
		FILE *fp = fopen(name.c_str(), "r");
		fseek(fp, start, 0);
		resp_size = fread(filedata, 1, req_size, fp);
		*/
	} else {
		File f = filelist[i];
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
			f.updateBlockInfo(b);
			resp_size = bsize - blockOffset;
			memcpy(resp_data, block_data + blockOffset, resp_size);
			return resp_data;
		}
	}
}

void initialize(string directory) {
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
		filelist.push_back(f);
	}
	closedir(dir);
}

int main() {
	string directory = "/home/deepthought/sandbox/p2pvideo/files";
	initialize(directory);
	int sockfd = bindToPort("127.0.0.1", streaming_port);
	char *cache = new char[100000000];
	while (listen(sockfd, 10) == 0) {
		/* listen and accept incoming connections */
		struct sockaddr_storage incoming;
		socklen_t incoming_sz = sizeof(incoming);
		long new_fd = accept(sockfd, (struct sockaddr *)&incoming, &incoming_sz);
		if (new_fd == -1) {
			cout<<"accept failed with error: "<<strerror(errno)<<endl;
		} else {
			cout<<"got connection: "<<new_fd<<endl;
			char header[1024];
			memset(header, 0, 1024);
			int chunk_size = 1000000;
			int filesize = readFile("test.mp4");
			int roundoff = (filesize / chunk_size) * chunk_size;
			cout<<"roundoff = "<<roundoff<<endl;
			int range_offset = 0;
			FILE *fp = fopen("test.mp4", "r");
			int offset = 0;
			char crlf[5] = "\r\n\r\n";
			while (1) {
				int bytes_recv = recv(new_fd, header + offset, 1, 0);
				//cout<<header[offset];
				if (offset >= 3) {
					if (memcmp(header + offset - 3, crlf, 4) == 0) {
						cout<<"done!"<<endl;
						cout<<"header:\n"<<header<<endl;
						int start, end;
						getRangeOffset(header, start, end);
						range_offset = start;
						int num_bytes;
						char *filedata = getBlock("pudhu.mp4", range_offset, 0, num_bytes, filesize);
						cout<<"block size = "<<num_bytes<<endl;
						char response[1024];
						int end_range = range_offset + num_bytes - 1;
						int header_bytes = sprintf(response, "HTTP/1.1 206 Partial Content\r\n"
							"Content-Type: video/mp4\r\nContent-Range: bytes "
							"%d-%d/%d\r\nTransfer-Encoding: chunked\r\n\r\n",
							range_offset, end_range, filesize);
						cout<<response<<endl;
						cout<<"num bytes = "<<num_bytes<<endl;
						cout<<"Bytes sent = "<<send(new_fd, response, header_bytes, 0)<<endl;

						int size_bytes = sprintf(response, "%x\r\n", num_bytes);
						cout<<response<<endl;
						cout<<"size bytes = "<<size_bytes<<" num_bytes = "<<num_bytes<<endl;
						send(new_fd, response, size_bytes, 0);
						cout<<"File bytes sent = "<<send(new_fd, filedata, num_bytes, 0)<<endl;
						size_bytes = sprintf(response, "\r\n0\r\n\r\n");
						cout<<response<<endl;
						send(new_fd, response, size_bytes, 0);
						delete[] filedata;
						offset = 0;
						continue;

						/*
						fseek(fp, range_offset, 0);
						char response[1024];
						char filedata[chunk_size];
						int end_range = (range_offset + chunk_size - 1 >= filesize - 1) ? (filesize - 1) : (range_offset + chunk_size - 1);
						int num_bytes = sprintf(response, "HTTP/1.1 206 Partial Content\r\n"
							"Content-Type: video/mp4\r\nContent-Range: bytes "
							"%d-%d/%ld\r\nTransfer-Encoding: chunked\r\n\r\n",
							range_offset, end_range, filesize);
						cout<<response<<endl;
						cout<<"num bytes = "<<num_bytes<<endl;
						cout<<"Bytes sent = "<<send(new_fd, response, num_bytes, 0)<<endl;
						while ((num_bytes = fread(filedata, 1, chunk_size, fp)) != 0) {
							int size_bytes = sprintf(response, "%x\r\n", num_bytes);
							cout<<response<<endl;
							cout<<"size bytes = "<<size_bytes<<" num_bytes = "<<num_bytes<<endl;
							send(new_fd, response, size_bytes, 0);
							cout<<"File bytes sent = "<<send(new_fd, filedata, num_bytes, 0)<<endl;
							size_bytes = sprintf(response, "\r\n0\r\n\r\n");
							cout<<response<<endl;
							send(new_fd, response, size_bytes, 0);
							range_offset += chunk_size;
							break;
						}
						offset = 0;
						continue;
						*/

					}
				} 
				offset++;
			}
		}
	}
	cout<<"exiting"<<endl;
	return 0;
}
