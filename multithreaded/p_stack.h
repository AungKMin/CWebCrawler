#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct p_stack_struct
{
    size_t size;               /* the max capacity of the PSTACK */
    size_t pos;                /* position of last item pushed onto the PSTACK */
    void** items;            /* PSTACK of strings */
} PSTACK;

int init_pstack(PSTACK *p, size_t PSTACK_size);
int is_full_ps(PSTACK *p);
int is_empty_ps(PSTACK *p);
int push_ps(PSTACK *p, void* item);
int pop_ps(PSTACK *p, void** p_item);
int resize_ps(PSTACK *p);
int cleanup_ps(PSTACK *p);