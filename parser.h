#include<libxml/parser.h>
#include<libxml/tree.h>
#include<string.h>

void getTrackerConfig(int* port, int* threads);

void getClientConfig(char** tracker_ip, int* tracker_port, int* own_ip, int *peer_port, int *stream_port, char** str_dir, int* block_size);
