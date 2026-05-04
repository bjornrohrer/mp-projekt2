#include <stdio.h>

#include "card.h"
#include "list.h"
#include "text_ui.h"

#define NUM_COLUMNS 7
#define NUM_FOUNDATIONS 4
#define COL_WIDTH 5

/* Render one card into a 2-char buffer.
 * Face-down cards are shown as "[]". */
static void format_card(Card card, char out[3]) {
    if (!card.face_up) {
        out[0] = '[';
        out[1] = ']';
    } else {
        out[0] = rank_to_char(card.rank);
        out[1] = suit_to_char(card.suit);
    }
    out[2] = '\0';
}

/* Print the column header row: C1   C2   C3 ... C7 */
static void print_column_header(void) {
    for (int i = 0; i < NUM_COLUMNS; i++) {
        printf("C%-*d", COL_WIDTH - 1, i + 1);
    }
    printf("\n");
}

/* Print one foundation slot inline: "F1:AS" or "F1:--". No newline. */
static void print_foundation(int index, CardNode *foundation) {
    printf("F%d:", index + 1);
    if (foundation == NULL) {
        printf("[]");
    } else {
        Card top = peek_tail(foundation);
        char buf[3];
        top.face_up = true;
        format_card(top, buf);
        printf("%s", buf);
    }
}

void print_gamestate(const Gamestate *gs) {
    if (gs == NULL) {
        return;
    }

    print_column_header();

    int max_height = 0;
    for (int i = 0; i < NUM_COLUMNS; i++) {
        int len = list_length(gs->columns[i]);
        if (len > max_height) {
            max_height = len;
        }
    }
    if (max_height < NUM_FOUNDATIONS) {
        max_height = NUM_FOUNDATIONS;
    }

    for (int row = 0; row < max_height; row++) {
        for (int col = 0; col < NUM_COLUMNS; col++) {
            int len = list_length(gs->columns[col]);
            if (row < len) {
                Card card = specific_node(gs->columns[col], row);
                char buf[3];
                format_card(card, buf);
                printf("%-*s", COL_WIDTH, buf);
            } else {
                printf("%-*s", COL_WIDTH, "");
            }
        }

        if (row < NUM_FOUNDATIONS) {
            printf("   ");
            print_foundation(row, gs->foundations[row]);
        }

        printf("\n");
    }
}

void print_deck(CardNode *deck) {
    CardNode *current = deck;
    while (current != NULL) {
        printf("%c%c\n",
               rank_to_char(current->card.rank),
               suit_to_char(current->card.suit));
        current = current->next;
    }
}

void print_message(const char *message) {
    if (message == NULL) {
        return;
    }
    printf("%s\n", message);
}
