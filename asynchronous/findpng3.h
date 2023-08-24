#include "curl_xml.h"
#include "hash.h"
#include <pthread.h>

#define URL_SIZE 512
#define STACK_SIZE 1024
#define HMAP_SIZE 1024
#define MAX_WAIT_MSECS 30*1000 /* Wait max. 30 seconds */