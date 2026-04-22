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
    } else {
        CardNode *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = node;
    }
}

/* Pop node from head */
Card pop_head(CardNode **head) {
    if (*head == NULL) {
        printf("The list is empty\n");
        Card empty = {0};
        return empty;
    }

    CardNode *temp = *head;
    Card value = temp->card;

    *head = (*head)->next;
    free(temp);

    return value;
}

/* Pop node from tail */
Card pop_tail(CardNode **head) {
    if (*head == NULL) {
        printf("The list is empty\n");
        Card empty = {0};
        return empty;
    }

    if ((*head)->next == NULL) {
        Card value = (*head)->card;
        free(*head);
        *head = NULL;
        return value;
    }

    /* Traverse to the second-to-last node */
    CardNode *current = *head;
    while (current->next->next != NULL) {
        current = current->next;
    }

    CardNode *tail = current->next;
    Card value = tail->card;
    current->next = NULL;
    free(tail);
    return value;
}
/* Look at head without removing it */

/* Look at tail without removing it */

/* Get number of nodes in list */

/* Get node at specific index */

/* Find node by number and suit of card */

/* Split list by specific node returning the tail */

/* Append an entire sublist to the tail of a list */

/* Free all nodes in a list */

