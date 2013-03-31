#include "common.h"
#include "assert.h"
#include <errno.h>
#include <pthread.h>
#define MAX_CLIENT_THREADS 7

int clients_port = 7501;  	// Each client listens on this port for incoming connections from other clients
int clients_sockfd;		// Client socket fd


// Function to transfer the file between peers
void * fileTransfer(void *clientsockfd)
{
}


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

int main(int argc, char **argv) {
	string ip = "localhost";
	int port = 5000;
	string tracker_ip = "127.0.0.1";
	int tracker_port = 7500;

	if (argc == 3) {
		ip = argv[1];
		port = atoi(argv[2]);
		ip += ":";
		ip += argv[2];
		cout<<ip<<endl;
	}
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
	registerWithTracker(sockfd, tclient);

	for (int i = 0; i < tnum; i++) {
		tmap[i] = true;
	}
	int file_idx = tclient.getFileIdxByURL(filename);
	tclient.updateFile(file_idx, tmap);
	updateOnTracker(sockfd, tclient);
	queryTracker(sockfd, tclient);

	string _ip = "127.0.0.1";
        clients_sockfd = bindToPort(_ip, clients_port);
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
                long new_fd = accept(sockfd, (struct sockaddr *)&incoming, &incoming_sz);
                if (new_fd == -1) 
		{
                        cout<<"accept failed with error: "<<strerror(errno)<<endl;
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
