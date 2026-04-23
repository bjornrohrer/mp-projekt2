#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "card.h"

CardNode *create_node(Card card) {
    CardNode *node = (CardNode *) malloc(sizeof(CardNode));
    if (node == NULL) {
        printf("Out of memory");
        exit(1);
    }
    node->card = card;
    node->next = NULL;
    return node;
}

void append_node(CardNode **head, Card card) {
    CardNode *new_node = create_node(card);
    if (new_node == NULL) {
        return;
    }

    if (*head == NULL) {
        *head = new_node;
        return;
    }

    CardNode *current = *head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = new_node;
}

void free_list(CardNode *head) {
    CardNode *current = head;
    CardNode *next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}

int load_deck(const char *filename, CardNode **deck) {
    FILE *file = fopen(filename, "r");
    char line[100];
    Card card;

    if (file == NULL) {
        return 0;
    }
    *deck = NULL;

    while (fgets(line, sizeof(line) file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        if (card_from_string(line, &card)) {
            append_node(deck, card);
        } else {
            fclose(file);
            free_list(*deck);
            *deck = NULL;
            return 0;
        }
    }

    fclose(file);
    return 1;
}
