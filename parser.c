#include "parser.h"

void getTrackerConfig(char** ip, int* port, int* threads)
{
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	doc = xmlReadFile("tracker.xml", NULL, 0);

	root_element = xmlDocGetRootElement(doc);

	xmlNode *cur_node = NULL;	
		
	for (cur_node = root_element->children; cur_node; cur_node = cur_node->next) 
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if(strcmp((char *)cur_node->name,"ip") == 0)
			{
				*ip = (char *)malloc(sizeof((char *)cur_node->children->content) * sizeof(char));
				strcpy(*ip, (char *)cur_node->children->content);
			}
			if(strcmp((char *)cur_node->name,"port") == 0)
			{
				*port = atoi((char *)cur_node->children->content);
			}
			if(strcmp((char *)cur_node->name,"threads") == 0)
			{
				*threads = atoi((char *)cur_node->children->content);
			}
	        }
    	}
}

void getClientConfig(char** tracker_ip, int* tracker_port, char** own_ip, int *peer_port, int *stream_port, char** str_dir, int* block_size)
{
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	doc = xmlReadFile("client.xml", NULL, 0);

	root_element = xmlDocGetRootElement(doc);

	xmlNode *cur_node = NULL;
		
	for (cur_node = root_element->children; cur_node; cur_node = cur_node->next) 
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if(strcmp((char *)cur_node->name,"tracker") == 0)
			{
				xmlNode *internalNode = NULL;
				for(internalNode = cur_node->children; internalNode; internalNode = internalNode->next)
				{
					if(strcmp((char *)internalNode->name, "ip") == 0)
					{
						*tracker_ip = (char *) malloc(sizeof((char *)internalNode->children->content) * sizeof(char));
						strcpy(*tracker_ip, (char *)internalNode->children->content);
						//tracker_ip = internalNode->children->content;
					}
					if(strcmp((char *)internalNode->name, "port") == 0)
					{
						*tracker_port = atoi((char *)internalNode->children->content);
					}					
				}
			}
			if(strcmp((char *)cur_node->name,"ip") == 0)
			{
				*own_ip = (char *) malloc(sizeof((char *)cur_node->children->content) * sizeof(char));
				strcpy(*own_ip, (char *)cur_node->children->content);
				//*own_ip = atoi(cur_node->children->content);
			}
			if(strcmp((char *)cur_node->name,"pport") == 0)
			{
				*peer_port = atoi((char *)cur_node->children->content);
			}
			if(strcmp((char *)cur_node->name,"sport") == 0)
			{
				*stream_port = atoi((char *)cur_node->children->content);
			}
			if(strcmp((char *)cur_node->name,"directory") == 0)
			{
				*str_dir = (char *)cur_node->children->content;
				strcpy(*str_dir, (char *)cur_node->children->content);
			}
			if(strcmp((char *)cur_node->name,"blocksize") == 0)
			{
				*block_size = atoi((char *)cur_node->children->content);
			}
	        }
    	}
}

/* void main()
{
	int tracker_port, own_ip, peer_port, stream_port, block_size;
	char *str_dir, *tracker_ip;
	getClientConfig(&tracker_ip, &tracker_port, &own_ip, &peer_port, &stream_port, &str_dir, &block_size);
	printf("tracker_ip: %s\ntracker_port: %d\nown_ip: %d\npeer_port: %d\nstream_port: %d\nstr_dir: %s\nblock_size: %d\n", tracker_ip, tracker_port, own_ip, peer_port, stream_port, str_dir, block_size);
} */
