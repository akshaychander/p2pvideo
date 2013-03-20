#include "tracker.h"
#include <pthread.h>
#include <assert.h>

#define MAX_THREADS	20
int port = 7500, sockfd;

Tracker::Tracker(int port) {
	this->port = port;
}

int
Tracker::getClientIdxByIP(const string& target) {
	for (int i = 0; i < p2pnodes.size(); i++) {
		if (p2pnodes[i].getIP() == target) {
			return i;
		}
	}
	return -1;
}

bool
Tracker::client_register(const Client& clnt) {
	if (getClientIdxByIP(clnt.getIP()) != -1) {
		cout<<"Duplicate entry!"<<endl;
		return false;
	}
	p2pnodes.push_back(clnt);
	return true;
}

bool
Tracker::update(Client& clnt, const File& file) {
	int idx = getClientIdxByIP(clnt.getIP());
	if (idx == -1) {
		cout<<"Invalid client!"<<endl;
		return false;
	}
	int file_idx = clnt.getFileIdxByURL(file.getURL());
	clnt.updateFile(file_idx, file.getBlockInfo());
}

void *
handleClient(void *clientsockfd) {
	long clientfd = (long)clientsockfd;
	cout<<"in thread for fd = "<<clientfd<<endl;
	char header[HEADER_SZ];
	// Receive requests from the client and send back data
	while (1) {
		memset(header, 0, HEADER_SZ);
		int bytes_rcvd = recv(clientfd, header, HEADER_SZ, 0);
		assert(bytes_rcvd == HEADER_SZ);
		int op, packet_size;
		memcpy((char *)&op, header, sizeof(int));
		memcpy((char *)&packet_size, header + sizeof(int), sizeof(int));
		cout<<"op = "<<op<<" packet_size = "<<packet_size<<endl;
		char data[packet_size];
		bytes_rcvd = recv(clientfd, data, packet_size, 0);
		assert(bytes_rcvd == packet_size);
		Client c;
		c.deserialize(data, packet_size);
		c.print();
	}
}

int main() {
	string ip = "127.0.0.1";
	sockfd = bindToPort(ip, port);
	if (sockfd != -1) {
		Tracker t(port);

	} else {
		cout<<"Could not create socket or bind to port"<<endl;
		abort();
		exit (1);
	}
	pthread_t threads[MAX_THREADS];
	int thread_id = 0;
	while (listen(sockfd, BACKLOG) == 0) {
		/* listen and accept incoming connections */
		struct sockaddr_storage incoming;
		socklen_t incoming_sz;
		long new_fd = accept(sockfd, (struct sockaddr *)&incoming, &incoming_sz);
		if (new_fd == -1) {
			cout<<"accept failed\n";
		} else {
			if( pthread_create(&threads[thread_id], NULL, handleClient, (void *)new_fd)) {
				cout<<"Thread creating failed"<<endl;
			}
		}
	}
	cout<<"exiting"<<endl;
	return 0;
}
