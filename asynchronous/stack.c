#include "stack.h"

int init_stack(STACK *p, size_t stack_size)
{
    if ( p == NULL || stack_size == 0 ) {
        return 1;
    }

    p->size = stack_size;
    p->pos  = -1;
    p->items = (char**) malloc(stack_size * sizeof(char*));
    memset(p->items, 0, stack_size * sizeof(char*));
    for (int i = 0; i < p->size; ++i) { 
        p->items[i] = NULL;
    }

    return 0;
}

int push_s(STACK *p, char* item)
{
    if ( p == NULL ) {
        return -1;
    }

    if (is_full_s(p)) { 
        resize_s(p);
    }

    ++(p->pos);
    p->items[p->pos] = malloc((strlen(item) + 1) * sizeof(char));
    memset(p->items[p->pos], 0, (strlen(item) + 1) * sizeof(char));
    strncpy(p->items[p->pos], item, strlen(item));

    return 0;
}

/**
 * @brief push one integer onto the stack 
 * @param ISTACK *p the address of the STACK data structure
 * @param int *item output parameter to save the integer value 
 *        that pops off the stack 
 * @return 0 on success; non-zero otherwise
 */

int pop_s(STACK *p, char **p_item)
{
    if ( p == NULL ) {
        return -1;
    }

    if (!is_empty_s(p) ) {
        *p_item = malloc(sizeof(char) * (strlen(p->items[p->pos]) + 1));
        memset(*p_item, 0, sizeof(char) * (strlen(p->items[p->pos]) + 1));
        strncpy(*p_item, p->items[p->pos], strlen(p->items[p->pos]));
        free(p->items[p->pos]);
        p->items[p->pos] = NULL;
        (p->pos)--;
        return 0;
    } else {
        return 1;
    }
}

/**
 * @brief check if the stack is full
 * @param ISTACK *p the address of the STACK data structure
 * @return non-zero if the stack is full; zero otherwise
 */

int is_full_s(STACK *p)
{
    if ( p == NULL ) {
        return 0;
    }
    return ( p->pos == (p->size -1) );
}

/**
 * @brief check if the stack is empty 
 * @param ISTACK *p the address of the STACK data structure
 * @return non-zero if the stack is empty; zero otherwise
 */

int is_empty_s(STACK *p)
{
    if ( p == NULL ) {
        return 0;
    }
    return ( p->pos == -1 );
}

int resize_s(STACK *p) { 
    size_t old_size = p->size;
    char **old_items = p->items;
    p->size = (p->size) * 2;
    p->items = (char**) malloc((p->size) * sizeof(char*));
    for (size_t i = 0; i < old_size; ++i) { 
        p->items[i] = old_items[i];
    }
    for (size_t i = old_size; i < p->size; ++i) { 
        p->items[i] = NULL;
    }

    free(old_items);
    old_items = NULL;

    return 0;
}

int cleanup_s(STACK* p) { 
    if (p == NULL) { 
        return 0;
    }
    if (p->items != NULL) { 
        for (size_t i = 0; i < p->size; ++i) { 
            if (p->items[i] != NULL) { 
                free(p->items[i]);
                p->items[i] = NULL;
            }
        }
        free(p->items);
        p->items = NULL;
    }
    return 0;
}