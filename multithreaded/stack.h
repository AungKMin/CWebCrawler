#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct stack
{
    size_t size;               /* the max capacity of the stack */
    size_t pos;                /* position of last item pushed onto the stack */
    char** items;            /* stack of strings */
} STACK;

int init_stack(STACK *p, size_t stack_size);
int is_full_s(STACK *p);
int is_empty_s(STACK *p);
int push_s(STACK *p, char* item);
int pop_s(STACK *p, char** p_item);
int resize_s(STACK *p);
int cleanup_s(STACK *p);