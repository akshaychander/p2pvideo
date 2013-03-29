#include "tracker.h"
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <map>

#define MAX_THREADS	20
int port = 7500, sockfd;
Tracker t(port);

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
Tracker::update(Client& clnt) {
	int idx = getClientIdxByIP(clnt.getIP());
	if (idx == -1) {
		cout<<"Invalid client!"<<endl;
		return false;
	}
	p2pnodes[idx] = clnt;
}

bool
Tracker::update(int idx, const File& file) {
	if (idx == -1) {
		cout<<"Invalid client!"<<endl;
		return false;
	}
	Client clnt = p2pnodes[idx];
	int file_idx = clnt.getFileIdxByURL(file.getURL());
	clnt.updateFile(file_idx, file.getBlockInfo());
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

char *
Tracker::serialize(int &size) {
	map<char *, int>tmp_data;
	size = sizeof(int);	//For number of clients
	for (int i = 0; i < p2pnodes.size(); i++) {
		int csize;
		char *cdata = p2pnodes[i].serialize(csize);
		cout<<"i = "<<i<<" csize = "<<csize<<endl;
		tmp_data.insert(pair<char *, int>(cdata, csize));
		size += sizeof(int) + csize; //size of serialized client and the actual data
	}
	char *data = new char[size];
	int offset = 0;
	int num_clients = p2pnodes.size();
	memcpy(data + offset, (char *)&num_clients, sizeof(int));
	offset += sizeof(int);
	map<char *, int>::iterator it;
	for (it = tmp_data.begin(); it != tmp_data.end(); it++) {
		char *cdata = it->first;
		int csize = it->second;
		cout<<"csize = "<<csize<<endl;

		memcpy(data + offset, (char *)&csize, sizeof(int));
		offset += sizeof(int);

		memcpy(data + offset, cdata, csize);
		offset += csize;
	}
	return data;
}

void
Tracker::deserialize(const char *data, const int& size) {
	// Not required, I think
}

void
Tracker::print() {
	cout<<"Port: "<<port<<endl;
	for (int i = 0; i < p2pnodes.size(); i++) {
		cout<<"Client "<<i<<endl;
		p2pnodes[i].print();
		cout<<endl;
	}
}
void *
handleClient(void *clientsockfd) {
	long clientfd = (long)clientsockfd;
	cout<<"in thread for fd = "<<clientfd<<endl;
	char header[HEADER_SZ];
	// Receive requests from the client and send back data
	while (1) {
		memset(header, 0, HEADER_SZ);

		/*
		 * Receive header (size is HEADER_SZ always).
		 * Contains Operation and Packet Size.
		 */
		int bytes_rcvd = recv(clientfd, header, HEADER_SZ, 0);
		assert(bytes_rcvd == HEADER_SZ);
		int op, packet_size;
		memcpy((char *)&op, header, sizeof(int));
		memcpy((char *)&packet_size, header + sizeof(int), sizeof(int));
		char data[packet_size];
		cout<<"op = "<<op<<" packet_size = "<<packet_size<<endl;
		Client c;
		switch (op) {

			/*
			 * Receive the client information
			 */
			case TRACKER_OP_REGISTER:
				bytes_rcvd = recv(clientfd, data, packet_size, 0);
				assert(bytes_rcvd == packet_size);
				cout<<"bytes_rcvd = "<<bytes_rcvd<<endl;
				c.deserialize(data, packet_size);
				c.print();
				t.client_register(c);
				t.print();
				break;

			case TRACKER_OP_QUERY:
				char *tdata;
				int tsize, bytes_sent;
				char response_header[HEADER_SZ];

				tdata = t.serialize(tsize);

				// op is not required, but makes things more consistent.
				memcpy(response_header, (char *)&op, sizeof(int));
				memcpy(response_header + sizeof(int), (char *)&tsize, sizeof(int));
				bytes_sent = send(clientfd, response_header, HEADER_SZ, 0);
				assert(bytes_sent == HEADER_SZ);
				bytes_sent = send(clientfd, tdata, tsize, 0);
				assert(bytes_sent == tsize);
				cout<<"Bytes sent = "<<bytes_sent<<endl;
				free(tdata);	//Only client serialize uses malloc
				break;


			case TRACKER_OP_UPDATE:
				/*
				int ip_length;
				string ip_addr;
				 //Get length of IP address followed by ip_address
				bytes_rcvd = recv(clientfd, (char *)&ip_length, sizeof(int), 0);
				assert(bytes_rcvd == sizeof(int));
				char ip[ip_length + 1];
				bytes_rcvd = recv(clientfd, ip, ip_length, 0);
				assert(bytes_rcvd == ip_length);
				ip[ip_length] = '\0';
				ip_addr = ip;

				int idx = t.getClientIdxByIP(ip_addr);
				if (idx == -1) {
					cout<<"Invalid client ip: "<<ip_addr<<endl;
					abort();
					exit(1);
				}
				File f;
				f.deserialize(data, packet_size);
				f.print();
				t.update(idx, f);
				*/
				bytes_rcvd = recv(clientfd, data, packet_size, 0);
				assert(bytes_rcvd == packet_size);
				cout<<"bytes_rcvd = "<<bytes_rcvd<<endl;
				c.deserialize(data, packet_size);
				c.print();
				t.update(c);
				t.print();
				break;
		}
	}
}

int main() {
	string ip = "127.0.0.1";
	sockfd = bindToPort(ip, port);
	if (sockfd == -1) {
		cout<<"Could not create socket or bind to port"<<endl;
		abort();
		exit (1);
	}
	pthread_t threads[MAX_THREADS];
	int thread_id = 0;
	while (listen(sockfd, BACKLOG) == 0) {
		/* listen and accept incoming connections */
		struct sockaddr_storage incoming;
		socklen_t incoming_sz = sizeof(incoming);
		long new_fd = accept(sockfd, (struct sockaddr *)&incoming, &incoming_sz);
		if (new_fd == -1) {
			cout<<"accept failed with error: "<<strerror(errno)<<endl;
		} else {
			if( pthread_create(&threads[thread_id], NULL, handleClient, (void *)new_fd)) {
				cout<<"Thread creating failed"<<endl;
			}
		}
	}
	cout<<"exiting"<<endl;
	return 0;
}
