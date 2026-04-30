#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "card.h"
#include "list.h"
#include "deck.h"

/* Loader deck fra fil */
int load_deck(const char *filename, CardNode **deck) {
    FILE *file = fopen(filename, "r");
    char line[100];
    Card card;
    CardNode *temp_deck = NULL;

    /* Holder styr på duplicates */
    int seen[4][13] = {0};
    int count = 0;

    if (file == NULL) {
        return 0;
    }

    /* Læser filen linje for linje */
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        /* Laver tekst om til kort */
        if (!card_from_string(line, &card)) {
            fclose(file);
            free_list(&temp_deck);
            return 0;
        }

        /* Tjekker om kortet allerede findes */
        if (seen[card.suit][card.rank - 1]) {
            fclose(file);
            free_list(&temp_deck);
            return 0;
        }

        seen[card.suit][card.rank - 1] = 1;
        count++;

        /* Laver node til kortet */
        CardNode *node = node_create(card);

        if (node == NULL) {
            fclose(file);
            free_list(&temp_deck);
            return 0;
        }

        /* Tilføjer kortet til decket */
        append(&temp_deck, node);
    }

    fclose(file);

    /* Decket skal have præcis 52 kort */
    if (count != 52) {
        free_list(&temp_deck);
        return 0;
    }

    /* Erstatter gammelt deck med nyt deck */
    free_list(deck);
    *deck = temp_deck;

    return 1;
}

/* Laver et nyt standard deck */
int generate_unshuffled_deck(CardNode **deck) {
    CardNode *temp_deck = NULL;

    /* Clubs */
    for (Rank rank = RANK_ACE; rank <= RANK_KING; rank++) {
        Card card;
        card.rank = rank;
        card.suit = SUIT_CLUBS;
        card.face_up = false;

        CardNode *node = node_create(card);

        if (node == NULL) {
            free_list(&temp_deck);
            return 0;
        }

        append(&temp_deck, node);
    }

    /* Diamonds */
    for (Rank rank = RANK_ACE; rank <= RANK_KING; rank++) {
        Card card;
        card.rank = rank;
        card.suit = SUIT_DIAMOND;
        card.face_up = false;

        CardNode *node = node_create(card);

        if (node == NULL) {
            free_list(&temp_deck);
            return 0;
        }

        append(&temp_deck, node);
    }

    /* Hearts */
    for (Rank rank = RANK_ACE; rank <= RANK_KING; rank++) {
        Card card;
        card.rank = rank;
        card.suit = SUIT_HEART;
        card.face_up = false;

        CardNode *node = node_create(card);

        if (node == NULL) {
            free_list(&temp_deck);
            return 0;
        }

        append(&temp_deck, node);
    }

    /* Spades */
    for (Rank rank = RANK_ACE; rank <= RANK_KING; rank++) {
        Card card;
        card.rank = rank;
        card.suit = SUIT_SPADE;
        card.face_up = false;

        CardNode *node = node_create(card);

        if (node == NULL) {
            free_list(&temp_deck);
            return 0;
        }

        append(&temp_deck, node);
    }

    free_list(deck);
    *deck = temp_deck;

    return 1;
}

/* Viser hele decket */
void show_deck(CardNode *deck) {
    CardNode *current = deck;

    while (current != NULL) {
        /* Vender kortet face up */
        current->card.face_up = true;

        /* Printer fx AS, TD eller KH */
        printf("%c%c\n",
               rank_to_char(current->card.rank),
               suit_to_char(current->card.suit));

        current = current->next;
    }
}

/* Gemmer decket i fil */
int save_deck(const char *filename, CardNode *deck) {
    FILE *file = fopen(filename, "w");
    CardNode *current = deck;

    if (file == NULL) {
        return 0;
    }

    /* Skriver hvert kort til filen */
    while (current != NULL) {
        fprintf(file, "%c%c\n",
                rank_to_char(current->card.rank),
                suit_to_char(current->card.suit));

        current = current->next;
    }

    fclose(file);
    return 1;
}

/* SI: splitter ved valgt kort og fletter delene */
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

    /* right starter ved det valgte kort */
    right = split_list(&left, rank, suit);

    if (right == NULL) {
        return;
    }

    /* Fletter skiftevis fra left og right */
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

    /* Tilføjer resten, hvis left har flere kort */
    if (left != NULL) {
        append_sublist(&shuffled, left);
    }

    /* Tilføjer resten, hvis right har flere kort */
    if (right != NULL) {
        append_sublist(&shuffled, right);
    }

    *deck = shuffled;
}

/* SR: blander decket tilfældigt */
void shuffle_random(CardNode **deck) {
    CardNode *shuffled = NULL;
    CardNode *current;
    CardNode *next;
    int length = 0;

    if (deck == NULL || *deck == NULL) {
        return;
    }

    srand((unsigned int) time(NULL));

    current = *deck;

    /* Indsætter hvert kort på en tilfældig position */
    while (current != NULL) {
        next = current->next;
        current->next = NULL;

        int position = rand() % (length + 1);

        /* Indsæt først */
        if (position == 0) {
            current->next = shuffled;
            shuffled = current;
        } else {
            CardNode *temp = shuffled;

            /* Gå frem til positionen */
            for (int i = 0; i < position - 1; i++) {
                temp = temp->next;
            }

            /* Indsæt kortet */
            current->next = temp->next;
            temp->next = current;
        }

        length++;
        current = next;
    }

    *deck = shuffled;
}