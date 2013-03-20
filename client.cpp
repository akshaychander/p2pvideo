#include "common.h"
#include "assert.h"

int main() {
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

	File tfile("testurl1234567890", tmap);
	//tfile.print();
	data = tfile.serialize(size);
	File dfile;
	dfile.deserialize(data, size);
	//dfile.print();
	Client tclient("localhost", 5000, "/tmp");
	tclient.addFile(tfile);
	//tclient.print();

	cout<<endl<<endl;

	Client dclient;
	int size2;
	char *data2 = tclient.serialize(size2);
	//dclient.deserialize(data2, size2);
	//dclient.print();
	
	struct addrinfo hints, *res;
	int sockfd;

	// first, load up address structs with getaddrinfo():

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo("127.0.0.1", "7500", &hints, &res);

	// make a socket:

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	// connect!

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		cout<<"Could not connect to tracker\n"<<endl;
		abort();
	}
	cout<<"Connected to tracker"<<endl;
	char header[HEADER_SZ];
	int op = TRACKER_OP_REGISTER;
	memcpy(header, (char *)&op, sizeof(int));
	memcpy(header + sizeof(int), (char *)&size2, sizeof(int));
	int bytes_sent = send(sockfd, header, HEADER_SZ, 0);
	assert(bytes_sent == HEADER_SZ);
	bytes_sent = send(sockfd, data2, size2, 0);
	assert(bytes_sent == size2);
	while(1);
	return 0;
}
