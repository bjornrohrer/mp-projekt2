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

// bruger bare to faser lige nu
typedef enum {
    PHASE_STARTUP,
    PHASE_PLAY
} Phase;

// startup
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
static int is_startup_command(const char *command);
static int handle_command(const char *command);

// kører command loop
int game_logic_run(void) {
    char command[40];
    int running = 1;

    while (running) {
        print_tableau(&current_game);
        printf("\nLAST Command: %s\n", last_command);
        printf("Message: %s\n", message);
        printf("INPUT > ");

        if (scanf("%39s", command) != 1) {
            printf("Input error\n");
            break;
        }

        strcpy(last_command, command);
        running = handle_command(command);
    }

    clear_tableau(&current_game);
    clear_foundations(&current_game);
    free_list(&current_deck);
    return 0;
}

// finder hvor rækken starter i Yukon layoutet
static int first_col_in_row(int row) {
    // første række starter helt til venstre
    if (row == 0) {
        return 0;
    }
    // række 1 og 2 starter i kolonne 2
    if (row <= 2) {
        return 1;
    }
    // række 3 til 5 starter i kolonne 3
    if (row <= 5) {
        return 2;
    }
    // række 6 til 8 starter i kolonne 4
    if (row <= 8) {
        return 3;
    }
    // de sidste rækker starter i kolonne 5
    return 4;
}

// rydder alle kolonner før vi dealer igen
static void clear_tableau(Gamestate *game) {
    int i;

    for (i = 0; i < COLS; i++) {
        free_list(&game->columns[i]);
    }
}

// rydder foundations når et nyt spil starter
static void clear_foundations(Gamestate *game) {
    int i;

    for (i = 0; i < FOUNDATIONS; i++) {
        free_list(&game->foundations[i]);
    }
}

// laver en ny node til linked list
static CardNode *new_node(Card card) {
    CardNode *node = malloc(sizeof(CardNode));

    if (node == NULL) {
        return NULL;
    }

    node->card = card;
    node->next = NULL;
    return node;
}

// lægger et kort nederst i en kolonne
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

// dealer kortene række for række
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

    // nulstiller først gammel state
    clear_tableau(game);
    clear_foundations(game);

    current = current_deck;

    // går gennem hele layoutet række for række
    for (row = 0; row < ROWS; row++) {
        start_col = first_col_in_row(row);

        // kun de kolonner der findes i rækken får et kort
        for (col = start_col; col < COLS; col++) {
            if (current == NULL) {
                return 0;
            }

            card = current->card;
            count_in_col[col]++;

            // de første kort i hver kolonne skal være skjulte
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

// printer et kort simpelt i terminalen
static void print_card_local(Card card) {
    if (!card.face_up) {
        printf("[]");
    } else {
        printf("%c%c", rank_char_local(card.rank), suit_char_local(card.suit));
    }
}

// printer tableauet i terminalen
static void print_tableau(const Gamestate *game) {
    int row;
    int col;
    CardNode *row_ptrs[COLS];
    int counts[COLS] = {0};
    CardNode *tmp;

    // tæller hvor mange kort der er i hver kolonne
    for (col = 0; col < COLS; col++) {
        row_ptrs[col] = game->columns[col];
        tmp = game->columns[col];
        while (tmp != NULL) {
            counts[col]++;
            tmp = tmp->next;
        }
    }

    printf("C1\tC2\tC3\tC4\tC5\tC6\tC7\n\n");

    // printer én række ad gangen
    for (row = 0; row < ROWS; row++) {
        for (col = 0; col < COLS; col++) {
            if (row < counts[col] && row_ptrs[col] != NULL) {
                print_card_local(row_ptrs[col]->card);
                row_ptrs[col] = row_ptrs[col]->next;
            }
            printf("\t");
        }

        // viser foundations ude til højre på nogle rækker
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

// tjekker om kommandoen hører til startup-fasen
static int is_startup_command(const char *command) {
    if (strcmp(command, "LD") == 0) return 1;
    if (strcmp(command, "SW") == 0) return 1;
    if (strcmp(command, "SI") == 0) return 1;
    if (strcmp(command, "SR") == 0) return 1;
    if (strcmp(command, "SD") == 0) return 1;
    if (strcmp(command, "QQ") == 0) return 1;
    return 0;
}

// håndterer kommandoerne for den nuværende del
static int handle_command(const char *command) {
    if (current_phase == PHASE_PLAY && is_startup_command(command)) {
        strcpy(message, "Command not available in the PLAY phase.");
        return 1;
    }

    if (strcmp(command, "LD") == 0) {
        clear_tableau(&current_game);
        clear_foundations(&current_game);
        free_list(&current_deck);
        current_phase = PHASE_STARTUP;
        if (!generate_unshuffled_deck(&current_deck)) {
            strcpy(message, "Could not load deck.");
            deck_loaded = 0;
            return 1;
        }

        deck_loaded = 1;
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
