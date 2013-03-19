#include "tracker.h"

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

	while (listen(sockfd, BACKLOG)) {
		/* listen and accept incoming connections */
		struct sockaddr_storage incoming;
		int incoming_sz = sizeof(incoming);
	}
	return 0;
}
