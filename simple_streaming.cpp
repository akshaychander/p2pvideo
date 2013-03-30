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
using namespace std;

/* Todo: Handle SIGPIPE */
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

// return file size in bytes
long readFile(char *name) {
	struct stat st;
	stat(name, &st);
	cout<<"File size = "<<st.st_size<<endl;
	return st.st_size;
}

int main() {
	int sockfd = bindToPort("127.0.0.1", 10000);

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
			int offset = 0;
			char crlf[5] = "\r\n\r\n";
			while (1) {
				int bytes_recv = recv(new_fd, header + offset, 1, 0);
				//cout<<header[offset];
				if (offset >= 3) {
					if (memcmp(header + offset - 3, crlf, 4) == 0) {
						cout<<"done!"<<endl;
						int filesize = readFile("pudhu.mp4");
						int roundoff = (filesize >> 5) << 5;
						cout<<"roundoff = "<<roundoff<<endl;
						int range_offset = 0;
						char response[1024];
						sprintf(buff,"HTTP/1.1 206 Partial
						Content\r\nContent-Type: video/mp4\r\nContent-Range: bytes
						0-1024/1915873\r\nTransfer-Encoding: chunked\r\nServer:
						Myserver\r\n\r\n%x\r\n",cnt);
						sprintf(
						while (range_offset < roundoff) {
							fread
						}
						for (int i = 0;
						break;
					}
				} 
				offset++;
			}
		}
	}
	cout<<"exiting"<<endl;
	return 0;
}
