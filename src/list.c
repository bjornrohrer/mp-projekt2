#include "card.h"
#include <stdio.h>
#include <stdlib.h>

/* Create node */
CardNode *node_create(Card card) {
    CardNode *node = malloc(sizeof(CardNode));
    if (node == NULL) {
        return NULL;
    }
    node->card = card;
    node->next = NULL;
    return node;
}

/* Push node to head */
void prepend(CardNode **head, CardNode *node) {
    node->next = *head;
    *head = node;
}

/* Push node to tail */
void append(CardNode **head, CardNode *node) {
    node->next = NULL;
    if (*head == NULL) {
        *head = node;
    }
}

/* Pop node from head */
int pop_head(CardNode *head) {
    if (*head == NULL) {
        printf("The list is empty\n");
    }

    CardNode *temp = *head;
    int data = temp->head;

    *head = (*head)->next;
    free(temp);

    return data;
}
/* Pop node from tail */

/* Look at head without removing it */

/* Look at tail without removing it */

/* Get number of nodes in list */

/* Get node at specific index */

/* Find node by number and suit of card */

/* Split list by specific node returning the tail */

/* Append an entire sublist to the tail of a list */

/* Free all nodes in a list */

