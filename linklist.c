#include <stdio.h>
#include <stdlib.h>
#include "linklist.h"

typedef struct node {
    int val;
    struct node * next;
} node_t;

node_t* init() {
	node_t * head = malloc(sizeof(node_t));
	if (head == NULL) {
	    return NULL;
	}
	
	head->val = 0;
	head->next = NULL;
	return head;
}

void push(node_t *head, int val) {
	node_t *temp = head;
	while (temp->next != NULL)
		temp = temp->next;
	temp->next = malloc(sizeof(node_t));
	temp->next->val = val;
	temp->next->next = NULL; 
}

int pop(node_t *head, int val) {
	node_t *temp = head;
	while (temp != NULL && temp->next != NULL) {
		if (temp->next->val == val) {
			node_t *temp2 = temp->next;
			temp->next = temp->next->next;
			free(temp2);
			return 0;
		} else {
			temp = temp -> next;
		}
	}
	return -1;
}

void print_list(node_t * head) {
    node_t * current = head;

    while (current != NULL) {
        printf("val: %d\n", current->val);
        current = current->next;
    }
}

void destroy(node_t *head) {
	if (head == NULL) {
		return;
	} else {
		destroy(head->next);
		free(head);
	}
}

