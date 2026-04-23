#include <stdio.h>
#include <string.h>

#include "card.h"
#include "list.h"

int load_deck(const char *filename, CardNode **deck) {
    FILE *file = fopen(filename, "r");
    char line[100];
    Card card;
    CardNode *temp_deck = NULL;

    if (file == NULL) {
        return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        if (!card_from_string(line, &card)) {
            fclose(file);
            free_list(&temp_deck);
            return 0;
        }

        CardNode *node = node_create(card);
        if (node == NULL) {
            fclose(file);
            free_list(&temp_deck);
            return 0;
        }

        append(&temp_deck, node);
    }

    fclose(file);

    if (list_length(temp_deck) != 52) {
        free_list(&temp_deck);
        return 0;
    }

    free_list(deck);
    *deck = temp_deck;
    return 1;
}

void show_deck(CardNode *deck) {
    CardNode *current = deck;

    while (current != NULL) {
        current->card.face_up = true;
        printf("%c%c\n",
               rank_to_char(current->card.rank),
               suit_to_char(current->card.suit));
        current = current->next;
    }
}

int save_deck(const char *filename, CardNode *deck) {
    FILE *file = fopen(filename, "w");
    CardNode *current = deck;

    if (file == NULL) {
        return 0;
    }

    while (current != NULL) {
        fprintf(file, "%c%c\n",
                rank_to_char(current->card.rank),
                suit_to_char(current->card.suit));
        current = current->next;
    }

    fclose(file);
    return 1;
}

void shuffle_interleave_from_card(CardNode **deck, Rank rank, Suit suit) {
    CardNode *left;
    CardNode *right;
    CardNode *shuffled = NULL;
    CardNode *next_left;
    CardNode *next_right;

    if (deck == NULL || *deck == NULL) {
        return;
    }

    left = *deck;
    right = split_list(&left, rank, suit);

    if (right == NULL) {
        return;
    }

    while (left != NULL && right != NULL) {
        next_left = left->next;
        left->next = NULL;
        append(&shuffled, left);
        left = next_left;

        next_right = right->next;
        right->next = NULL;
        append(&shuffled, right);
        right = next_right;
    }

    if (left != NULL) {
        append_sublist(&shuffled, left);
    }

    if (right != NULL) {
        append_sublist(&shuffled, right);
    }

    *deck = shuffled;
}