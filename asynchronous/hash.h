#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include "p_stack.h"

typedef struct hashmap
{
    size_t size;               
    char** elements;
    struct hsearch_data *hmap;
    size_t cur_size;  
    PSTACK* ps;
} HMAP;

int init_hmap(HMAP *p, size_t map_size);
int is_full_h(HMAP *p);
int is_empty_h(HMAP *p);
int add_h(HMAP *p, char* key);
int search_h(HMAP *p, char* key); /* return 1 on found and 0 on not found */
int resize_h(HMAP *p);
int cleanup_h(HMAP *p);