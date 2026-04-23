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
Card peek_head(CardNode *head) {
    if (head == NULL) {
        Card empty = {0};
        return empty;
    }

    return head->card;
}

/* Look at tail without removing it */
Card peek_tail(CardNode *head) {
    if (head == NULL) {
        Card empty = {0};
        return empty;
    }

    if (head->next == NULL) {
        return head->card;
    }

    CardNode *current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    return current->card;
}

/* Get number of nodes in list */
int list_length(CardNode *head) {
    if (head == NULL) {
        return 0;
    }

    if (head->next == NULL) {
        return 1;
    }

    int count = 0;
    CardNode *current = head;
    while (current != NULL) {
        count += 1;
        current = current->next;
    }

    return count;
}

/* Get node at specific index */
Card specific_node(CardNode *head, int index) {
    if (index < 0) {
        Card empty = {0};
        return empty;
    }

    CardNode *current = head;
    while (index > 0 && current != NULL) {
        index -= 1;
        current = current->next;
    }

    if (current == NULL) {
        Card empty = {0};
        return empty;
    }

    return current->card;
}

/* Find node by number and suit of card */
Card find_node(CardNode *head, Rank rank, Suit suit) {
    CardNode *current = head;
    while (current != NULL) {
        if (current->card.rank == rank && current->card.suit == suit) {
            return current->card;
        }
        current = current->next;
    }

    Card empty = {0};
    return empty;
}

/* Split list by specific node returning the tail */
CardNode *split_list(CardNode **head, Rank rank, Suit suit) {
    if (*head == NULL) {
        return NULL;
    }

    if ((*head)->card.rank == rank && (*head)->card.suit == suit) {
        CardNode *tail = *head;
        *head = NULL;
        return tail;
    }

    CardNode *previous = *head;
    CardNode *current = previous->next;
    while (current != NULL) {
        if (current->card.rank == rank && current->card.suit == suit) {
            previous->next = NULL;
            return current;
        }
        previous = current;
        current = current->next;
    }

    return NULL;
}

/* Append an entire sublist to the tail of a list */
void append_sublist(CardNode **head, CardNode *sublist) {
    if (sublist == NULL) {
        return;
    }

    if (*head == NULL) {
        *head = sublist;
        return;
    }

    CardNode *current = *head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = sublist;
}

/* Free all nodes in a list */

