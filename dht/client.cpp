#include "common.h"
#include <pthread.h>
#include <iostream>
#include "parser.h"
#include <sys/time.h>

#define MAX_CLIENT_THREADS 7

int clients_port = 7501;  	// Each client listens on this port for incoming connections from other clients
int clients_sockfd;		// Client socket fd
int tracker_fd;
string dht_tracker_ip;
int streaming_port = 10000;

string storage_directory;

Client c;
extern int cache_size;
extern int tracker_port;
pthread_rwlock_t client_mutex;
extern Tracker t;
extern void handleClientTracker(void *fd, Tracker& track);
void signalHandler(int signum, siginfo_t *info, void *ptr) {
	switch (signum) {
		case SIGPIPE:
			//Just ignore
			break;

		case SIGTERM:
		case SIGINT:
			c.disconnect();
			exit(0);
	}
}
/*
 * Function to transfer the file between peers. Receives request for a block and
 * sends that block. Closes connection after transfer is complete.
 */

void *
fileTransfer(void *clientsockfd) {
	long sockfd = (long)clientsockfd;
	char header[HEADER_SZ];
	//int bytes_rcvd = recv(sockfd, header, HEADER_SZ, 0);
	//assert(bytes_rcvd == HEADER_SZ);
	recvSocketData(sockfd, HEADER_SZ, header);
	int op, reqsize;
	int offset = 0;
	int urllen, blocknum;

	memcpy((char *)&op, header, sizeof(int));
	offset += sizeof(int);
	memcpy((char *)&reqsize, header + offset, sizeof(int));
	offset = 0;

	//cout<<"OP = "<<op<<" Req size = "<<reqsize<<endl;
	char* data = new char[reqsize];
	//bytes_rcvd = recv(sockfd, data, reqsize, 0);
	recvSocketData(sockfd,reqsize, data);
	if (op == CLIENT_REQ_DATA) {
		memcpy((char *)&blocknum, data + offset, sizeof(int));
		offset += sizeof(int);

		memcpy((char *)&urllen, data + offset, sizeof(int));
		offset += sizeof(int);

		char requrl[urllen + 1];

		memcpy(requrl, data + offset, urllen);
		//cout<<"url len = "<<urllen<<endl;
		offset += urllen;
		requrl[urllen] = '\0';
		string url = requrl;
		cout<<"Peer has requested block "<<blocknum<<" for file "<<url<<endl;
		c.sendBlock(sockfd, url, blocknum);
	}
	close(sockfd);
	return NULL;
}

/*
// Returns socketfd on success
int connectToTracker(string tracker_ip, int port) {
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
		return sockfd;
	}
	// connect!

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		cout<<"Could not connect to tracker\n"<<endl;
		exit(1);
		abort();
	}
	return sockfd;
}

void registerWithTracker(int sockfd, const Client& clnt) {
	char header[HEADER_SZ];
	int op = TRACKER_OP_REGISTER;
	char *data;
	int size;

	data = clnt.serialize(size);
	memcpy(header, (char *)&op, sizeof(int));
	memcpy(header + sizeof(int), (char *)&size, sizeof(int));
	int bytes_sent = send(sockfd, header, HEADER_SZ, 0);
	assert(bytes_sent == HEADER_SZ);
	bytes_sent = send(sockfd, data, size, 0);
	assert(bytes_sent == size);
	cout<<"Bytes sent = "<<bytes_sent<<endl;
	free(data);	//Only client serialize uses malloc
}

void updateOnTracker(int sockfd, const Client& clnt) {
	char header[HEADER_SZ];
	int op = TRACKER_OP_UPDATE;
	char *data;
	int size;

	data = clnt.serialize(size);
	memcpy(header, (char *)&op, sizeof(int));
	memcpy(header + sizeof(int), (char *)&size, sizeof(int));
	int bytes_sent = send(sockfd, header, HEADER_SZ, 0);
	assert(bytes_sent == HEADER_SZ);
	bytes_sent = send(sockfd, data, size, 0);
	assert(bytes_sent == size);
	cout<<"Bytes sent = "<<bytes_sent<<endl;
	free(data);	//Only client serialize uses malloc
}

void queryTracker(int sockfd, Client& clnt) {
	char header[HEADER_SZ];
	int op = TRACKER_OP_QUERY;
	char *data;
	int size = 1;	//packet size doesnt matter for query

	memcpy(header, (char *)&op, sizeof(int));
	memcpy(header + sizeof(int), (char *)&size, sizeof(int));
	int bytes_sent = send(sockfd, header, HEADER_SZ, 0);
	assert(bytes_sent == HEADER_SZ);

	//Now, receive info from tracker

	int bytes_rcvd = recv(sockfd, header, HEADER_SZ, 0); 
	assert(bytes_rcvd == HEADER_SZ);

	//Ignore op field in response and take the packet size
	memcpy((char *)&size, header + sizeof(int), sizeof(int));
	data = new char[size];
	bytes_rcvd = recv(sockfd, data, size, 0);
	assert(bytes_rcvd == size);
	cout<<"bytes_rcvd = "<<bytes_rcvd<<endl;

	int num_clients, offset = 0;
	memcpy((char *)&num_clients, data + offset, sizeof(int));
	offset += sizeof(int);

	for (int i = 0; i < num_clients; i++) {
		int csize;
		memcpy((char *)&csize, data + offset, sizeof(int));
		offset += sizeof(int);
		Client c;
		c.deserialize(data + offset, csize);
		offset += csize;
		c.print();
	}
	delete[] data;

}
*/

struct pargs {
	char name[256];
	int offset;
	int bsize;
	int fsize;
};

void *prefetcher(void *args) {
	struct pargs *myargs = (struct pargs *)args;
	for (int i = 0; i < 2; i++) {
		int startoffset = c.getPrefetchOffset(myargs->name, myargs->offset, myargs->bsize, myargs->fsize);
		if (startoffset != -1) {
			char *filedata = c.getBlock(myargs->name, startoffset, 0, myargs->bsize, myargs->fsize);
			delete[] filedata;
		}
		//myargs->offset = startoffset + ((10 + (rand() % 5)) * myargs->bsize);
		myargs->offset = startoffset + ((10 + (time(NULL) % 5)) * myargs->bsize);
	}
}
void *
handleStreaming(void *param) {
	int sockfd = bindToPort("127.0.0.1", streaming_port);
	while (listen(sockfd, 10) == 0) {
		/* listen and accept incoming connections */
		struct sockaddr_storage incoming;
		socklen_t incoming_sz = sizeof(incoming);
		long new_fd = accept(sockfd, (struct sockaddr *)&incoming, &incoming_sz);
		if (new_fd == -1) {
			cout<<"accept failed with error: "<<strerror(errno)<<endl;
			exit(1);
		} else {
			//cout<<"got connection: "<<new_fd<<endl;
			char header[1024];
			memset(header, 0, 1024);
			int chunk_size = 1000000;
			int filesize = readFile("test.mp4");
			int roundoff = (filesize / chunk_size) * chunk_size;
			//cout<<"roundoff = "<<roundoff<<endl;
			int range_offset = 0;
			FILE *fp = fopen("test.mp4", "r");
			int offset = 0;
			char crlf[5] = "\r\n\r\n";
			while (1) {
				//int bytes_recv = recv(new_fd, header + offset, 1, 0);
				recvSocketData(new_fd, 1, header + offset);
				//cout<<header[offset];
				if (offset >= 3) {
					if (memcmp(header + offset - 3, crlf, 4) == 0) {
						//cout<<"done!"<<endl;
						//cout<<"header:\n"<<header<<endl;
						int start, end;
						getRangeOffset(header, start, end);
						char *requrl = getFileName(header);
						range_offset = start;
						int num_bytes;
						char *filedata = c.getBlock(requrl, range_offset, 0, num_bytes, filesize);
						//cout<<"block size = "<<num_bytes<<endl;
						char response[1024];
						int end_range = range_offset + num_bytes - 1;
						int header_bytes = sprintf(response, "HTTP/1.1 206 Partial Content\r\n"
							"Content-Type: video/webm\r\nContent-Range: bytes "
							"%d-%d/%d\r\nTransfer-Encoding: chunked\r\n\r\n",
							range_offset, end_range, filesize);
						//cout<<response<<endl;
						//cout<<"num bytes = "<<num_bytes<<endl;
						//cout<<"Bytes sent = "<<send(new_fd, response, header_bytes, 0)<<endl;
						sendSocketData(new_fd, header_bytes, response);
						int size_bytes = sprintf(response, "%x\r\n", num_bytes);
						//cout<<response<<endl;
						//cout<<"size bytes = "<<size_bytes<<" num_bytes = "<<num_bytes<<endl;
						//send(new_fd, response, size_bytes, 0);
						sendSocketData(new_fd, size_bytes, response);
						//cout<<"File bytes sent = "<<send(new_fd, filedata, num_bytes, 0)<<endl;
						sendSocketData(new_fd, num_bytes, filedata);
						size_bytes = sprintf(response, "\r\n0\r\n\r\n");
						//cout<<response<<endl;
						//send(new_fd, response, size_bytes, 0);
						sendSocketData(new_fd, size_bytes, response);
						delete[] filedata;
						offset = 0;
						//int newstart = range_offset + (3 + (rand() % 5)) * num_bytes;
						//int newstart = range_offset + (17) * num_bytes;
						int newstart = range_offset + num_bytes;
						if (newstart < filesize) {
							//cout<<"Prefetching random block with offset "<<newstart<<endl;
							struct pargs args;
							sprintf(args.name, "%s", requrl);
							args.offset = newstart;
							args.bsize = num_bytes;
							args.fsize = filesize;
							//filedata = c.getBlock(requrl, newstart, 0, num_bytes, filesize);
							pthread_t tmpt;
							pthread_create(&tmpt, NULL, prefetcher, (void *)&args);
						}
					}
				} 
				offset++;
			}
		}
	}
}

void *
handleQuery(void *param) {
	// This should fetch info from its own tracker
	while (1) {
		pthread_rwlock_wrlock(&client_mutex);
		c.setTrackerFd(FIRST_NODE);
		//c.queryTracker();
		c.querySelfTracker();
		pthread_rwlock_unlock(&client_mutex);
		sleep(3);
	}

}

void *
handleUpdate(void *param) {
	while (1) {
		pthread_rwlock_rdlock(&client_mutex);
		c.updateOnTracker();
		pthread_rwlock_unlock(&client_mutex);
		sleep(3);
	}
}

void *
handleStats(void *param) {
	while (1) {
		pthread_rwlock_rdlock(&client_mutex);
		c.printStats();
		pthread_rwlock_unlock(&client_mutex);
		sleep(STATS_SLEEP_TIME);
	}
}

void *
handleTracker(void *param) {
	string ip = dht_tracker_ip;
	/*
	char *xmlip;
	int numthreads;
	getTrackerConfig(&xmlip, &port, &numthreads); 
	ip = xmlip;
	*/
	int sockfd = bindToPort(ip, tracker_port);
	if (sockfd == -1) {
		cout<<"Could not create socket or bind to port"<<endl;
		abort();
		exit (1);
	}
	int thread_id = 0;
	while (listen(sockfd, BACKLOG) == 0) {
		struct sockaddr_storage incoming;
		socklen_t incoming_sz = sizeof(incoming);
		long new_fd = accept(sockfd, (struct sockaddr *)&incoming, &incoming_sz);
		if (new_fd == -1) {
			cout<<"accept failed with error: "<<strerror(errno)<<endl;
		} else {
			/*
			pthread_t tmpthread;
			if( pthread_create(&tmpthread, NULL, handleClient, (void *)new_fd)) {
				cout<<"Thread creating failed"<<endl;
			}
			else
			{
				thread_id++;
			}
			*/
			handleClientTracker((void *)new_fd, t);
			close(new_fd);
		}
	}
	cout<<"exiting"<<endl;

}

int main(int argc, char **argv) {
	struct sigaction handler;
	handler.sa_sigaction = signalHandler;
	handler.sa_flags = SA_SIGINFO;
	sigaction(SIGINT, &handler, NULL);
	sigaction(SIGPIPE, &handler, NULL);
	sigaction(SIGTERM, &handler, NULL);
	string ip = "localhost";
	int port = 5000;
	string tracker_ip = "127.0.0.1";
	//int tracker_port = 7500;
	storage_directory = "/home/deepthought/sandbox/p2pvideo/files";
	char *tip, *dir, *cip;
	int bsize;
	getClientConfig(&tip, &tracker_port, &cip, &clients_port, &streaming_port, &dir, &bsize, &cache_size);
	tracker_ip = tip;
	ip = cip;
	dht_tracker_ip = ip;
	storage_directory = dir;
	if (argc == 5) {
		ip = argv[1];
		port = atoi(argv[2]);
		storage_directory = argv[3];
		streaming_port = atoi(argv[4]);
		cout<<"storage directory is "<<storage_directory<<endl;
		clients_port = port;

		/*
		ip += ":";
		ip += argv[2];
		*/
		cout<<ip<<endl;
	}

	/*
	vector<bool> tmap;
	int tnum = 5;
	for (int i = 0; i < tnum; i++) {
		bool val = (i % 2 == 0);
		tmap.push_back(val);
	}

	BlockMap tblock(tmap);
	//tblock.print();

	int size;
	char *data = tblock.serialize(size);
	BlockMap dblock;
	dblock.deserialize(data, size);
	//dblock.print();

	string filename = "testurl1234567890";
	File tfile(filename, tmap);
	//tfile.print();
	data = tfile.serialize(size);
	File dfile;
	dfile.deserialize(data, size);
	//dfile.print();
	Client tclient(ip, port, "/tmp");
	tclient.addFile(tfile);
	//tclient.print();

	cout<<endl<<endl;

	Client dclient;
	int size2;
	char *data2 = tclient.serialize(size2);
	//dclient.deserialize(data2, size2);
	//dclient.print();

	int sockfd = connectToTracker(tracker_ip, tracker_port);
	if (sockfd != -1) {
		cout<<"Connected to tracker"<<endl;
	}
	//registerWithTracker(sockfd, tclient);

	for (int i = 0; i < tnum; i++) {
		tmap[i] = true;
	}
	int file_idx = tclient.getFileIdxByURL(filename);
	//tclient.updateFile(file_idx, tmap);
	//updateOnTracker(sockfd, tclient);
	*/
	pthread_rwlock_init(&client_mutex, NULL);
	Client tmpc(ip, clients_port, storage_directory);
	c = tmpc;
	c.connectToTracker(tracker_ip, tracker_port);
	c.registerWithTracker();
	c.connectToTracker(tracker_ip, tracker_port);
	c.queryTracker();
	c.client_mutex = &client_mutex;
	pthread_t streamer, queryThread, updateThread, statsThread, trackerThread;
	pthread_create(&streamer, NULL, handleStreaming, NULL);
	pthread_create(&queryThread, NULL, handleQuery, NULL);
	pthread_create(&updateThread, NULL, handleUpdate, NULL);
	pthread_create(&statsThread, NULL, handleStats, NULL);
	pthread_create(&trackerThread, NULL, handleTracker, NULL);
	cout<<"ip = "<<ip<<" port = "<<clients_port<<endl;
	clients_sockfd = bindToPort(ip, clients_port);
	if(clients_sockfd == -1)
	{
		cout << "Could not create a socket for incoming client connections" << endl;
		abort();
		exit(1);
	}
	pthread_t client_threads[MAX_CLIENT_THREADS];
	int thread_id = 0;
	while(listen(clients_sockfd, BACKLOG) == 0)
	{
		struct sockaddr_storage incoming;
		socklen_t incoming_sz = sizeof(incoming);
		long new_fd = accept(clients_sockfd, (struct sockaddr *)&incoming, &incoming_sz);
		if (new_fd == -1) 
		{
			cout<<"peer accept failed with error: "<<strerror(errno)<<endl;
			exit(1);
		}
		else
		{
			if(pthread_create(&client_threads[thread_id], NULL, fileTransfer, (void *)new_fd)) 
			{
				cout<<"Thread creating failed"<<endl;
			}
			else
			{
				thread_id++;
			}
		}
	}
	while(1);
	return 0;
}
