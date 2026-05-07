#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "card.h"
#include "deck.h"
#include "input.h"
#include "list.h"

#define ROWS 11
#define COLS 7
#define FOUNDATIONS 4
#define DECK_SIZE 52

typedef enum {
    PHASE_STARTUP,
    PHASE_PLAY
} Phase;

static Phase current_phase = PHASE_STARTUP;
static Gamestate current_game = {0};
static CardNode *current_deck = NULL;
static int deck_loaded = 0;

static int first_col_in_row(int row);
static void clear_tableau(Gamestate *game);
static void clear_foundations(Gamestate *game);
static void reset_board(void);
static int deal_from_current_deck(Gamestate *game);
static void format_card_token(Card card, char out[3]);
static void print_state(void);
static bool cards_match(Card left, Card right);
static CardNode *tail_node(CardNode *head);
static CardNode *find_card_node(CardNode *head, Card target);
static void reveal_exposed_tail(CardNode *column);
static bool can_move_to_foundation(Card card, CardNode *foundation);
static bool can_move_to_column(Card card, CardNode *column);
static bool move_card_to_foundation(Location from, Location to, char *message, size_t message_size);
static bool move_card_to_column(Location from, Location to, char *message, size_t message_size);
static void respond_status(const char *status, const char *message, bool include_state);
static void handle_load(const char *arg);
static void handle_shuffle_random(void);
static void handle_print(void);
static void handle_move(const char *command);

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

static void clear_tableau(Gamestate *game) {
    int i;

    for (i = 0; i < COLS; i++) {
        free_list(&game->columns[i]);
    }
}

static void clear_foundations(Gamestate *game) {
    int i;

    for (i = 0; i < FOUNDATIONS; i++) {
        free_list(&game->foundations[i]);
    }
}

static void reset_board(void) {
    clear_tableau(&current_game);
    clear_foundations(&current_game);
    current_phase = PHASE_STARTUP;
}

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
            CardNode *node;

            if (current == NULL) {
                return 0;
            }

            card = current->card;
            count_in_col[col]++;
            card.face_up = count_in_col[col] > col;

            node = node_create(card);
            if (node == NULL) {
                return 0;
            }

            append(&game->columns[col], node);
            current = current->next;
            index++;
        }
    }

    return index == DECK_SIZE;
}

static void format_card_token(Card card, char out[3]) {
    if (!card.face_up) {
        out[0] = 'X';
        out[1] = 'X';
    } else {
        out[0] = rank_to_char(card.rank);
        out[1] = suit_to_char(card.suit);
    }
    out[2] = '\0';
}

static void print_line_for_pile(const char *prefix, int index, CardNode *pile) {
    CardNode *current = pile;
    int first = 1;

    printf("%s%d:", prefix, index + 1);
    while (current != NULL) {
        char token[3];

        format_card_token(current->card, token);
        if (!first) {
            printf(",");
        }
        printf("%s", token);
        first = 0;
        current = current->next;
    }
    printf("\n");
}

static void print_state(void) {
    int i;

    for (i = 0; i < COLS; i++) {
        print_line_for_pile("C", i, current_game.columns[i]);
    }

    for (i = 0; i < FOUNDATIONS; i++) {
        print_line_for_pile("F", i, current_game.foundations[i]);
    }
}

static bool cards_match(Card left, Card right) {
    return left.rank == right.rank && left.suit == right.suit;
}

static CardNode *tail_node(CardNode *head) {
    CardNode *current = head;

    if (current == NULL) {
        return NULL;
    }

    while (current->next != NULL) {
        current = current->next;
    }

    return current;
}

static CardNode *find_card_node(CardNode *head, Card target) {
    CardNode *current = head;

    while (current != NULL) {
        if (cards_match(current->card, target)) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

static void reveal_exposed_tail(CardNode *column) {
    CardNode *tail = tail_node(column);

    if (tail != NULL && !tail->card.face_up) {
        tail->card.face_up = true;
    }
}


static bool can_move_to_foundation(Card card, CardNode *foundation) {
    if (foundation == NULL) {
        return card.rank == RANK_ACE;
    }

    Card top = peek_tail(foundation);
    return card.rank == top.rank + 1 && card.suit == top.suit;
}

static bool can_move_to_column(Card card, CardNode *column) {
    Card top;

    if (column == NULL) {
        return card.rank == RANK_KING;
    }

    top = peek_tail(column);
    if (!top.face_up) {
        return false;
    }

    return card.suit != top.suit && card.rank + 1 == top.rank;
}

static bool move_card_to_foundation(Location from, Location to, char *message, size_t message_size) {
    CardNode **source;
    CardNode **foundation;
    Card top;
    CardNode *node;

    if (from.index < 0 || from.index >= COLS || to.index < 0 || to.index >= FOUNDATIONS) {
        snprintf(message, message_size, "Move outside valid board range.");
        return false;
    }

    source = &current_game.columns[from.index];
    foundation = &current_game.foundations[to.index];

    if (*source == NULL) {
        snprintf(message, message_size, "Selected column is empty.");
        return false;
    }

    top = peek_tail(*source);
    if (!top.face_up) {
        snprintf(message, message_size, "Top card is face down.");
        return false;
    }

    if (from.kind == LOC_COL_CARD && !cards_match(from.card, top)) {
        snprintf(message, message_size, "Only the exposed top card can move.");
        return false;
    }

    if (!can_move_to_foundation(top, *foundation)) {
        snprintf(message, message_size, "Illegal move.");
        return false;
    }

    node = node_create(top);
    if (node == NULL) {
        snprintf(message, message_size, "Could not allocate move node.");
        return false;
    }

    pop_tail(source);
    append(foundation, node);
    reveal_exposed_tail(*source);
    snprintf(message, message_size, "OK");
    return true;
}

static bool move_card_to_column(Location from, Location to, char *message, size_t message_size) {
    CardNode **source;
    CardNode **target;
    Card top;
    CardNode *moving = NULL;
    Card moving_card;

    if (from.index < 0 || from.index >= COLS || to.index < 0 || to.index >= COLS) {
        snprintf(message, message_size, "Move outside valid board range.");
        return false;
    }

    if (from.index == to.index) {
        snprintf(message, message_size, "Source and destination columns are the same.");
        return false;
    }

    source = &current_game.columns[from.index];
    target = &current_game.columns[to.index];

    if (*source == NULL) {
        snprintf(message, message_size, "Selected column is empty.");
        return false;
    }

    if (from.kind == LOC_COL_CARD) {
        moving = find_card_node(*source, from.card);
        if (moving == NULL) {
            snprintf(message, message_size, "Selected card was not found.");
            return false;
        }
        if (!moving->card.face_up) {
            snprintf(message, message_size, "Selected card is face down.");
            return false;
        }
        moving_card = moving->card;
    } else {
        top = peek_tail(*source);
        if (!top.face_up) {
            snprintf(message, message_size, "Top card is face down.");
            return false;
        }
        moving_card = top;
    }

    if (!can_move_to_column(moving_card, *target)) {
        if (*target == NULL) {
            snprintf(message, message_size, "Only a king can move to an empty column.");
        } else {
            snprintf(message, message_size, "Illegal column move.");
        }
        return false;
    }

    if (from.kind == LOC_COL_CARD) {
        moving = split_list(source, moving_card.rank, moving_card.suit);
        if (moving == NULL) {
            snprintf(message, message_size, "Selected card was not found.");
            return false;
        }
        append_sublist(target, moving);
    } else {
        CardNode *node = node_create(moving_card);

        if (node == NULL) {
            snprintf(message, message_size, "Could not allocate move node.");
            return false;
        }

        pop_tail(source);
        append(target, node);
    }

    reveal_exposed_tail(*source);
    snprintf(message, message_size, "OK");
    return true;
}

static void respond_status(const char *status, const char *message, bool include_state) {
    printf("%s", status);
    if (message != NULL && message[0] != '\0') {
        printf(" %s", message);
    }
    printf("\n");

    if (include_state) {
        print_state();
    }

    printf("END\n");
    fflush(stdout);
}

static void handle_load(const char *arg) {
    reset_board();
    free_list(&current_deck);

    if (arg == NULL || arg[0] == '\0') {
        if (!generate_unshuffled_deck(&current_deck)) {
            deck_loaded = 0;
            respond_status("ERROR", "Could not load deck.", false);
            return;
        }
    } else if (!load_deck(arg, &current_deck)) {
        deck_loaded = 0;
        respond_status("ERROR", "Could not load deck.", false);
        return;
    }

    deck_loaded = 1;
    respond_status("OK", "", false);
}

static void handle_shuffle_random(void) {
    if (!deck_loaded || current_deck == NULL) {
        respond_status("ERROR", "No deck loaded.", false);
        return;
    }

    if (current_phase != PHASE_STARTUP) {
        respond_status("ERROR", "Command not available in the PLAY phase.", true);
        return;
    }

    shuffle_random(&current_deck);
    respond_status("OK", "", false);
}

static void handle_print(void) {
    if (!deck_loaded) {
        respond_status("ERROR", "No deck loaded.", false);
        return;
    }

    if (current_phase == PHASE_STARTUP) {
        if (!deal_from_current_deck(&current_game)) {
            respond_status("ERROR", "Error while dealing cards.", false);
            return;
        }
        current_phase = PHASE_PLAY;
    }

    respond_status("OK", "", true);
}

static void handle_move(const char *command) {
    Location from;
    Location to;
    char message[128];

    if (current_phase != PHASE_PLAY) {
        respond_status("ERROR", "Command not available in the STARTUP phase.", false);
        return;
    }

    if (!parse_move(command, &from, &to)) {
        respond_status("ERROR", "Malformed move.", true);
        return;
    }

    if (from.kind != LOC_COL_TAIL && from.kind != LOC_COL_CARD) {
        respond_status("ERROR", "Only column moves are implemented.", true);
        return;
    }

    if (to.kind == LOC_FOUNDATION) {
        if (!move_card_to_foundation(from, to, message, sizeof(message))) {
            respond_status("ERROR", message, true);
            return;
        }
    } else if (to.kind == LOC_COL_TAIL) {
        if (!move_card_to_column(from, to, message, sizeof(message))) {
            respond_status("ERROR", message, true);
            return;
        }
    } else {
        respond_status("ERROR", "Move destination is not supported.", true);
        return;
    }

    respond_status("OK", "", true);
}

int main(void) {
    char input[256];

    while (fgets(input, sizeof(input), stdin) != NULL) {
        char *newline = strchr(input, '\n');
        char *space;
        char *command;
        char *arg = NULL;

        if (newline != NULL) {
            *newline = '\0';
        }

        command = input;
        while (*command == ' ' || *command == '\t') {
            command++;
        }

        if (*command == '\0') {
            respond_status("ERROR", "Empty command.", current_phase == PHASE_PLAY);
            continue;
        }

        space = strpbrk(command, " \t");
        if (space != NULL) {
            *space = '\0';
            arg = space + 1;
            while (*arg == ' ' || *arg == '\t') {
                arg++;
            }
            if (*arg == '\0') {
                arg = NULL;
            }
        }

        if (strcmp(command, "LD") == 0) {
            handle_load(arg);
        } else if (strcmp(command, "SR") == 0) {
            handle_shuffle_random();
        } else if (strcmp(command, "P") == 0) {
            handle_print();
        } else if (strcmp(command, "QQ") == 0) {
            respond_status("OK", "", false);
            break;
        } else if (strstr(command, "->") != NULL) {
            handle_move(command);
        } else {
            respond_status("ERROR", "Command not available yet.", current_phase == PHASE_PLAY);
        }
    }

    reset_board();
    free_list(&current_deck);
    return 0;
}
