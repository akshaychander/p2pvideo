#include "tracker.h"

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
	return 0;
}
