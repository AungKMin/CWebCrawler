#include "hash.h"

// Note: I create new memory for the strings in add and search because hsearch keeps the memory passed in to it via item as its own  

int init_hmap(HMAP *p, size_t map_size) { 
    // create new hashmap
    p->hmap = malloc(sizeof(struct hsearch_data));
    memset(p->hmap, 0, sizeof(struct hsearch_data));
    if (hcreate_r(map_size, p->hmap) == 0) { 
        perror("hcreate\n");
        return -1;
    }

    //create array of strings
    p->elements = (char**) malloc(sizeof(char*) * map_size);
    for (int i = 0; i < map_size; ++i) { 
        p->elements[i] = NULL;
    }

    p->cur_size = 0;
    p->size = map_size;

    // create stack of pointers we need to free after matching searches
    p->ps = malloc(sizeof(PSTACK));
    memset(p->ps, 0, sizeof(PSTACK));
    init_pstack(p->ps, 1);

    return 0;
}

int is_full_h(HMAP* p) { 
    return (p->size == p->cur_size);
}

int is_empty_h(HMAP* p) { 
    return (p->cur_size == 0);
}

int add_h(HMAP* p, char* key) { 
    // resize if full
    if (is_full_h(p)) {
        resize_h(p); 
    }

    // add to hashmap
    ENTRY item;
    item.key = malloc(strlen(key) * (sizeof(char) + 1));
    memset(item.key, 0, strlen(key) * (sizeof(char) + 1));
    strncpy(item.key, key, strlen(key));
    item.data = NULL;
    ACTION action = ENTER;
    ENTRY *retval = NULL;
    if (hsearch_r(item, action, &retval, p->hmap) == 0) { 
        perror("hsearch_r\n");
        return -1;
    }

    // add new string to array of strings
    p->elements[p->cur_size] = item.key;
    ++p->cur_size;

    return 0;
}

int search_h(HMAP* p, char* key) { 

    // search in hashmap
    ENTRY item;
    item.key = malloc(strlen(key) * (sizeof(char) + 1));
    memset(item.key, 0, strlen(key) * (sizeof(char) + 1));
    strncpy(item.key, key, strlen(key));
    item.data = NULL;
    ACTION action = FIND;
    ENTRY *retval;

    if (hsearch_r(item, action, &retval, p->hmap) == 0) { 
        // perror("hsearch_r\n");
        // return -1;
    }

    if (retval != NULL) { 
        push_ps(p->ps, item.key);
        return 1;
    } else { 
        free(item.key);
        item.key = NULL;
        return 0;
    }

    return 0;    
}

int resize_h(HMAP* p) { 

    // destroy 
    hdestroy_r(p->hmap);
    free(p->hmap);
    p->hmap = NULL;

    // reinitialize
    p->hmap = malloc(sizeof(struct hsearch_data));
    memset(p->hmap, 0, sizeof(struct hsearch_data));
    if (hcreate_r(2*(p->size), p->hmap) == 0) { 
        perror("hcreate\n");
        return -1;
    }
    
    size_t old_size = p->size;
    char **old_elements = p->elements;
    p->size = (p->size) * 2;
    p->elements = (char**) malloc((p->size) * sizeof(char*));

    p->cur_size = 0;
    for (size_t i = 0; i < old_size; ++i) { 
        add_h(p, old_elements[i]);
        free(old_elements[i]);
        old_elements[i] = NULL;
    }
    for (size_t i = old_size; i < p->size; ++i) { 
        p->elements[i] = NULL;
    }

    free(old_elements);
    old_elements = NULL;

    return 0;
}

int cleanup_h(HMAP* p) { 
    for (size_t i = 0; i < p->size; ++i) { 
        if (p->elements[i] != NULL) {
            free(p->elements[i]);
            p->elements[i] = NULL;
        }
    }
    free(p->elements);
    p->elements = NULL;

    hdestroy_r(p->hmap);
    free(p->hmap);
    p->hmap = NULL;

    // clean up the pstack, which cleans up the extra string pointers created during search
    cleanup_ps(p->ps);
    free(p->ps);

    return 0;
}