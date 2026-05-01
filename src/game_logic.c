#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "card.h"
#include "deck.h"
#include "list.h"

#define ROWS 11
#define COLS 7
#define FOUNDATIONS 4
#define DECK_SIZE 52

// spillet har to faser
typedef enum {
    PHASE_STARTUP,
    PHASE_PLAY
} Phase;

// game state
static Phase current_phase = PHASE_STARTUP;
// spillets kolonner og foundations
static Gamestate current_game = {0};
static CardNode *current_deck = NULL;
static int deck_loaded = 0;
static char last_command[40] = "";
static char message[100] = "";

static int first_col_in_row(int row);
static void clear_tableau(Gamestate *game);
static void clear_foundations(Gamestate *game);
static CardNode *new_node(Card card);
static int append_card(CardNode **head, Card card);
static int deal_from_current_deck(Gamestate *game);
static char rank_char_local(Rank rank);
static char suit_char_local(Suit suit);
static void print_card_local(Card card);
static void print_tableau(const Gamestate *game);
static int show_current_deck(void);
static bool can_move_to_foundation(Card card, CardNode *foundation);
static int is_startup_command(const char *command);
static int handle_command(const char *command, const char *arg);

// kører input loopet
int game_logic_run(void) {
    char input[140];
    char command[40];
    char arg[100];
    int running = 1;
    int parts;

    while (running) {
        print_tableau(&current_game);
        printf("\nLAST Command: %s\n", last_command);
        printf("Message: %s\n", message);
        printf("INPUT > ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Input error\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        parts = sscanf(input, "%39s %99s", command, arg);
        if (parts < 1) {
            strcpy(message, "Command not available yet.");
            continue;
        }

        strcpy(last_command, input);

        if (parts == 1) {
            running = handle_command(command, NULL);
        } else {
            running = handle_command(command, arg);
        }
    }

    clear_tableau(&current_game);
    clear_foundations(&current_game);
    free_list(&current_deck);
    return 0;
}

// finder startkolonnen for en række
static int first_col_in_row(int row) {
    if (row == 0) {
        return 0;
    }
    if (row <= 2) {
        return 1;
    }
    if (row <= 5) {
        return 2;
    }
    if (row <= 8) {
        return 3;
    }
    return 4;
}

// rydder kolonnerne
static void clear_tableau(Gamestate *game) {
    int i;

    for (i = 0; i < COLS; i++) {
        free_list(&game->columns[i]);
    }
}

// rydder foundations
static void clear_foundations(Gamestate *game) {
    int i;

    for (i = 0; i < FOUNDATIONS; i++) {
        free_list(&game->foundations[i]);
    }
}

// laver en node
static CardNode *new_node(Card card) {
    CardNode *node = malloc(sizeof(CardNode));

    if (node == NULL) {
        return NULL;
    }

    node->card = card;
    node->next = NULL;
    return node;
}

// tilføjer kort til slutningen af listen
static int append_card(CardNode **head, Card card) {
    CardNode *node;
    CardNode *current;

    node = new_node(card);
    if (node == NULL) {
        return 0;
    }

    if (*head == NULL) {
        *head = node;
        return 1;
    }

    current = *head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = node;
    return 1;
}

// deler kort ud
static int deal_from_current_deck(Gamestate *game) {
    int row;
    int col;
    int start_col;
    int index = 0;
    int count_in_col[COLS] = {0};
    CardNode *current;
    Card card;

    if (!deck_loaded || current_deck == NULL) {
        return 0;
    }

    if (list_length(current_deck) != DECK_SIZE) {
        return 0;
    }

    clear_tableau(game);
    clear_foundations(game);

    current = current_deck;

    for (row = 0; row < ROWS; row++) {
        start_col = first_col_in_row(row);

        for (col = start_col; col < COLS; col++) {
            if (current == NULL) {
                return 0;
            }

            card = current->card;
            count_in_col[col]++;

            // skjuler de øverste kort
            if (count_in_col[col] <= col) {
                card.face_up = false;
            } else {
                card.face_up = true;
            }

            if (!append_card(&game->columns[col], card)) {
                return 0;
            }

            current = current->next;
            index++;
        }
    }

    return index == DECK_SIZE;
}

static char rank_char_local(Rank rank) {
    switch (rank) {
        case RANK_ACE: return 'A';
        case RANK_TWO: return '2';
        case RANK_THREE: return '3';
        case RANK_FOUR: return '4';
        case RANK_FIVE: return '5';
        case RANK_SIX: return '6';
        case RANK_SEVEN: return '7';
        case RANK_EIGHT: return '8';
        case RANK_NINE: return '9';
        case RANK_TEN: return 'T';
        case RANK_JACK: return 'J';
        case RANK_QUEEN: return 'Q';
        case RANK_KING: return 'K';
        default: return '?';
    }
}

static char suit_char_local(Suit suit) {
    switch (suit) {
        case SUIT_CLUBS: return 'C';
        case SUIT_DIAMOND: return 'D';
        case SUIT_HEART: return 'H';
        case SUIT_SPADE: return 'S';
        default: return '?';
    }
}

// printer kort
static void print_card_local(Card card) {
    if (!card.face_up) {
        printf("[]");
    } else {
        printf("%c%c", rank_char_local(card.rank), suit_char_local(card.suit));
    }
}

// printer bordet
static void print_tableau(const Gamestate *game) {
    int row;
    int col;
    CardNode *row_ptrs[COLS];
    int counts[COLS] = {0};
    CardNode *tmp;

    for (col = 0; col < COLS; col++) {
        row_ptrs[col] = game->columns[col];
        tmp = game->columns[col];
        while (tmp != NULL) {
            counts[col]++;
            tmp = tmp->next;
        }
    }

    printf("C1\tC2\tC3\tC4\tC5\tC6\tC7\n\n");

    for (row = 0; row < ROWS; row++) {
        for (col = 0; col < COLS; col++) {
            if (row < counts[col] && row_ptrs[col] != NULL) {
                print_card_local(row_ptrs[col]->card);
                row_ptrs[col] = row_ptrs[col]->next;
            }
            printf("\t");
        }

        // foundations til højre
        if (row == 0) {
            printf("[] F1");
        } else if (row == 2) {
            printf("[] F2");
        } else if (row == 4) {
            printf("[] F3");
        } else if (row == 6) {
            printf("[] F4");
        }

        printf("\n");
    }
}

// viser deck
static int show_current_deck(void) {
    Gamestate temp_game = {0};
    CardNode *current;
    Card card;
    int row;
    int col;
    int start_col;
    int index = 0;

    if (!deck_loaded || current_deck == NULL) {
        return 0;
    }

    if (list_length(current_deck) != DECK_SIZE) {
        return 0;
    }

    current = current_deck;

    for (row = 0; row < ROWS; row++) {
        start_col = first_col_in_row(row);

        for (col = start_col; col < COLS; col++) {
            if (current == NULL) {
                clear_tableau(&temp_game);
                return 0;
            }

            card = current->card;
            card.face_up = true;

            if (!append_card(&temp_game.columns[col], card)) {
                clear_tableau(&temp_game);
                return 0;
            }

            current = current->next;
            index++;
        }
    }

    print_tableau(&temp_game);
    clear_tableau(&temp_game);

    return index == DECK_SIZE;
}

// Regler for foundations
static bool can_move_to_foundation(Card card, CardNode *foundation) {
    if (foundation == NULL) {
        return card.rank == RANK_ACE;
    }

    Card top = peek_tail(foundation);

    return card.rank == top.rank + 1 && card.suit == top.suit;
}

// todo: Function move_card_to_foundation
// Card card might have to be CardNode *card to append
static Card move_card_to_foundation(Card card, CardNode **foundation) {
    Card top = peek_tail(*foundation);
    // Append the card to the tail of foundation
    if (can_move_to_foundation(card, *foundation)) {
         top = card;
         return top;
            }

    return top;
}
// startup kommandoer
static int is_startup_command(const char *command) {
    if (strcmp(command, "LD") == 0) return 1;
    if (strcmp(command, "SW") == 0) return 1;
    if (strcmp(command, "SI") == 0) return 1;
    if (strcmp(command, "SR") == 0) return 1;
    if (strcmp(command, "SD") == 0) return 1;
    if (strcmp(command, "QQ") == 0) return 1;
    return 0;
}

// håndterer input
static int handle_command(const char *command, const char *arg) {
    if (current_phase == PHASE_PLAY && is_startup_command(command)) {
        strcpy(message, "Command not available in the PLAY phase.");
        return 1;
    }

    if (strcmp(command, "LD") == 0) {
        clear_tableau(&current_game);
        clear_foundations(&current_game);
        free_list(&current_deck);
        current_phase = PHASE_STARTUP;

        if (arg == NULL) {
            if (!generate_unshuffled_deck(&current_deck)) {
                strcpy(message, "Could not load deck.");
                deck_loaded = 0;
                return 1;
            }
        } else {
            if (!load_deck(arg, &current_deck)) {
                strcpy(message, "Could not load deck.");
                deck_loaded = 0;
                return 1;
            }
        }

        deck_loaded = 1;
        strcpy(message, "OK");
        return 1;
    }

    if (strcmp(command, "SW") == 0) {
        if (!show_current_deck()) {
            strcpy(message, "No deck loaded.");
            return 1;
        }

        strcpy(message, "OK");
        return 1;
    }

    if (strcmp(command, "P") == 0) {
        if (current_phase == PHASE_PLAY) {
            strcpy(message, "Command not available in the PLAY phase.");
            return 1;
        }

        if (!deck_loaded) {
            strcpy(message, "No deck loaded.");
            return 1;
        }

        if (!deal_from_current_deck(&current_game)) {
            strcpy(message, "Error while dealing cards");
            return 1;
        }

        current_phase = PHASE_PLAY;
        strcpy(message, "OK");
        return 1;
    }

    if (strcmp(command, "Q") == 0) {
        if (current_phase == PHASE_PLAY) {
            clear_tableau(&current_game);
            clear_foundations(&current_game);
            current_phase = PHASE_STARTUP;
            strcpy(message, "OK");
        } else {
            strcpy(message, "Command not available in the STARTUP phase.");
        }
        return 1;
    }

    if (strcmp(command, "QQ") == 0) {
        strcpy(message, "OK");
        return 0;
    }

    strcpy(message, "Command not available yet.");

    return 1;
}
