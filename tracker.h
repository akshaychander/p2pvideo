#include "common.h"

class Tracker {
	vector<Client> p2pnodes;
	int port;

	public:
		Tracker(int port);
		int getClientIdxByIP(const string& target);
		bool client_register(const Client& clnt);
		bool update(Client& clnt, const File& file);
		BlockMap query(const File& file);
};
