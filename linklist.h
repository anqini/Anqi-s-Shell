#include <stdio.h>
#include <stdlib.h>

typedef struct node node_t;

node_t* init();

void push(node_t *head, int val);

int pop(node_t *head, int val);

void print_list(node_t *head);

void destroy(node_t *head);

