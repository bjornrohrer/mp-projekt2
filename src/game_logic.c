#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "card.h"
#include "deck.h"
#include "list.h"
#include "input.h"
#include "text_ui.h"

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
static int show_deck_view = 0;
static char last_command[40] = "";
static char message[160] = "";

static int first_col_in_row(int row);
static void clear_tableau(Gamestate *game);
static void clear_foundations(Gamestate *game);
static int deal_from_current_deck(Gamestate *game);
static int show_current_deck(void);
static int show_loaded_deck_hidden(void);
static bool can_move_to_foundation(Card card, CardNode *foundation);
static int move_card_to_foundation(CardNode **source, CardNode **foundation);
static int move_selected_card_to_foundation(CardNode **source, CardNode **foundation, Card selected);
static int move_foundation_to_column(CardNode **foundation, CardNode **destination);
static void flip_new_bottom_card(CardNode **column);
static bool can_place_on_column(Card card, CardNode *destination);
static int move_bottom_card_to_column(CardNode **source, CardNode **destination);
static int move_stack_to_column(CardNode **source, CardNode **destination, Card selected);
static int is_game_won(void);
static void set_move_message(void);
static int is_startup_command(const char *command);
static int handle_command(const char *command, const char *arg);
#ifdef GAME_LOGIC_BACKEND
static void backend_format_card(Card card, char out[3]);
static void backend_print_pile(const char *prefix, int index, CardNode *pile);
static void backend_print_state(void);
static void backend_print_deck_view(void);
static void backend_print_allowed_command(int *first, const char *command);
static void backend_print_column_card_command(int *first, int column, Card card, const char *target);
static void backend_print_allowed_commands(void);
static void backend_print_response(void);
static int game_logic_backend_run(void);
#endif

// kører input loopet
int game_logic_run(void) {
    char input[140];
    char command[40];
    char arg[100];
    int running = 1;
    int parts;

    while (running) {
        if (show_deck_view) {
            show_current_deck();
            show_deck_view = 0;
        } else {
            print_gamestate(&current_game);
        }
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

        snprintf(last_command, sizeof(last_command), "%s", input);

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
    if (row <= 5) {
        return 1;
    }
    if (row == 6) {
        return 2;
    }
    if (row == 7) {
        return 3;
    }
    if (row == 8) {
        return 4;
    }
    if (row == 9) {
        return 5;
    }
    return 6;
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

            CardNode *node = node_create(card);
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

// viser deck
static int show_current_deck(void) {
    Gamestate temp_game = {0};
    CardNode *current;
    Card card;
    int row;
    int col;
    int index = 0;

    if (!deck_loaded || current_deck == NULL) {
        return 0;
    }

    if (list_length(current_deck) != DECK_SIZE) {
        return 0;
    }

    current = current_deck;

    for (row = 0; row < ROWS && current != NULL; row++) {
        for (col = 0; col < COLS && current != NULL; col++) {
            card = current->card;
            card.face_up = true;

            CardNode *node = node_create(card);
            if (node == NULL) {
                clear_tableau(&temp_game);
                return 0;
            }
            append(&temp_game.columns[col], node);

            current = current->next;
            index++;
        }
    }

    print_gamestate(&temp_game);
    clear_tableau(&temp_game);

    return index == DECK_SIZE;
}

// viser loaded deck som skjulte kort
static int show_loaded_deck_hidden(void) {
    Gamestate temp_game = {0};
    CardNode *current;
    Card card;
    int row;
    int col;
    int index = 0;

    if (!deck_loaded || current_deck == NULL) {
        return 0;
    }

    if (list_length(current_deck) != DECK_SIZE) {
        return 0;
    }

    current = current_deck;

    for (row = 0; row < ROWS && current != NULL; row++) {
        for (col = 0; col < COLS && current != NULL; col++) {
            card = current->card;
            card.face_up = false;

            CardNode *node = node_create(card);
            if (node == NULL) {
                clear_tableau(&temp_game);
                return 0;
            }

            append(&temp_game.columns[col], node);
            current = current->next;
            index++;
        }
    }

    clear_tableau(&current_game);
    current_game = temp_game;

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

static int move_card_to_foundation(CardNode **source, CardNode **foundation) {
    if (*source == NULL) {
        return 0;
    }

    Card top = peek_tail(*source);

    if (!top.face_up) {
        return 0;
    }

    if (!can_move_to_foundation(top, *foundation)) {
        return 0;
    }

    CardNode *node = node_create(top);

    if (node == NULL) {
        return 0;
    }

    pop_tail(source);

    flip_new_bottom_card(source);

    append(foundation, node);
    return 1;
}
// flytter valgt kort til foundation hvis det er nederst
static int move_selected_card_to_foundation(CardNode **source, CardNode **foundation, Card selected) {
    CardNode *current;

    if (*source == NULL) {
        return 0;
    }

    current = *source;
    while (current != NULL) {
        if (current->card.rank == selected.rank && current->card.suit == selected.suit) {
            break;
        }
        current = current->next;
    }

    if (current == NULL) {
        return 0;
    }

    if (current->next != NULL) {
        return 0;
    }

    return move_card_to_foundation(source, foundation);
}
// flytter fra foundation til kolonne
static int move_foundation_to_column(CardNode **foundation, CardNode **destination) {
    if (*foundation == NULL) {
        return 0;
    }

    Card card = peek_tail(*foundation);

    if (!can_place_on_column(card, *destination)) {
        return 0;
    }

    CardNode *node = node_create(card);
    if (node == NULL) {
        return 0;
    }

    pop_tail(foundation);
    append(destination, node);

    return 1;
}
// vender nyt bundkort hvis det var skjult
static void flip_new_bottom_card(CardNode **column) {
    CardNode *tail = tail_node(*column);

    if (tail != NULL && !tail->card.face_up) {
        tail->card.face_up = true;
    }
}

// tjekker om et kort må ligge nederst i en kolonne
static bool can_place_on_column(Card card, CardNode *destination) {
    if (destination == NULL) {
        return card.rank == RANK_KING;
    }

    Card bottom = peek_tail(destination);

    return bottom.rank == card.rank + 1 && bottom.suit != card.suit;
}

// flytter nederste kort fra en kolonne til en anden
static int move_bottom_card_to_column(CardNode **source, CardNode **destination) {
    if (*source == NULL) {
        return 0;
    }

    Card card = peek_tail(*source);

    if (!card.face_up) {
        return 0;
    }

    if (!can_place_on_column(card, *destination)) {
        return 0;
    }

    CardNode *node = node_create(card);
    if (node == NULL) {
        return 0;
    }

    pop_tail(source);
    flip_new_bottom_card(source);
    append(destination, node);

    return 1;
}
// flytter fra et bestemt kort og ned
static int move_stack_to_column(CardNode **source, CardNode **destination, Card selected) {
    CardNode *current;
    CardNode *sublist;

    if (*source == NULL) {
        return 0;
    }

    current = *source;
    while (current != NULL) {
        if (current->card.rank == selected.rank && current->card.suit == selected.suit) {
            break;
        }
        current = current->next;
    }

    if (current == NULL) {
        return 0;
    }

    if (!current->card.face_up) {
        return 0;
    }

    if (!can_place_on_column(current->card, *destination)) {
        return 0;
    }

    sublist = split_list(source, selected.rank, selected.suit);
    if (sublist == NULL) {
        return 0;
    }

    append_sublist(destination, sublist);
    flip_new_bottom_card(source);

    return 1;
}

// startup kommandoer
static int is_startup_command(const char *command) {
    if (strcmp(command, "LD") == 0) return 1;
    if (strcmp(command, "SW") == 0) return 1;
    if (strcmp(command, "SI") == 0) return 1;
    if (strcmp(command, "SR") == 0) return 1;
    if (strcmp(command, "SD") == 0) return 1;
    return 0;
}

// tager imod input
static int handle_command(const char *command, const char *arg) {
    if (strcmp(command, "QQ") == 0) {
        strcpy(message, "OK");
        return 0;
    }
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
                snprintf(message, sizeof(message), "Could not load deck.");
                deck_loaded = 0;
                return 1;
            }
        } else {
            if (!load_deck(arg, &current_deck)) {
                const char *error = deck_last_error();

                if (error != NULL && error[0] != '\0') {
                    snprintf(message, sizeof(message), "%s", error);
                } else {
                    snprintf(message, sizeof(message), "Could not load deck.");
                }
                deck_loaded = 0;
                return 1;
            }
        }

        deck_loaded = 1;
        if (!show_loaded_deck_hidden()) {
            strcpy(message, "Could not show deck.");
            return 1;
        }
        strcpy(message, "OK");
        return 1;
    }

    if (strcmp(command, "SW") == 0) {
        if (!deck_loaded || current_deck == NULL) {
            strcpy(message, "No deck loaded.");
            return 1;
        }

        if (list_length(current_deck) != DECK_SIZE) {
            strcpy(message, "No deck loaded.");
            return 1;
        }

        show_deck_view = 1;
        strcpy(message, "OK");
        return 1;
    }

    if (strcmp(command, "SD") == 0) {
        const char *filename = arg;

        if (!deck_loaded || current_deck == NULL) {
            strcpy(message, "No deck loaded.");
            return 1;
        }

        if (filename == NULL) {
            filename = "cards.txt";
        }

        if (!save_deck(filename, current_deck)) {
            const char *error = deck_last_error();

            if (error != NULL && error[0] != '\0') {
                snprintf(message, sizeof(message), "%s", error);
            } else {
                snprintf(message, sizeof(message), "Could not save deck.");
            }
            return 1;
        }

        strcpy(message, "OK");
        return 1;
    }

    if (strcmp(command, "SI") == 0) {
        int split;
        int length;

        if (!deck_loaded || current_deck == NULL) {
            strcpy(message, "No deck loaded.");
            return 1;
        }

        length = list_length(current_deck);

        if (arg == NULL) {
            srand((unsigned int) time(NULL));
            split = (rand() % (length - 1)) + 1;
        } else {
            if (sscanf(arg, "%d", &split) != 1) {
                strcpy(message, "Invalid split.");
                return 1;
            }
        }

        if (!shuffle_interleave_split(&current_deck, split)) {
            strcpy(message, "Invalid split.");
            return 1;
        }

        strcpy(message, "OK");
        return 1;
    }

    if (strcmp(command, "SR") == 0) {
        if (!deck_loaded || current_deck == NULL) {
            strcpy(message, "No deck loaded.");
            return 1;
        }

        shuffle_random(&current_deck);
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

        clear_tableau(&current_game);

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


    if (strstr(command, "->") != NULL) {
        if (current_phase != PHASE_PLAY) {
            strcpy(message, "Command not available in the STARTUP phase.");
            return 1;
        }

        Location from, to;
        if (!parse_move(command, &from, &to)) {
            strcpy(message, "Malformed move.");
            return 1;
        }

        if (from.kind == LOC_COL_TAIL && to.kind == LOC_FOUNDATION) {
            if (!move_card_to_foundation(&current_game.columns[from.index],
                                         &current_game.foundations[to.index])) {
                strcpy(message, "Illegal move.");
                return 1;
            }

            set_move_message();
            return 1;
        }

        if (from.kind == LOC_COL_CARD && to.kind == LOC_FOUNDATION) {
            if (!move_selected_card_to_foundation(&current_game.columns[from.index],
                                                  &current_game.foundations[to.index],
                                                  from.card)) {
                strcpy(message, "Illegal move.");
                return 1;
            }

            set_move_message();
            return 1;
        }

        if (from.kind == LOC_FOUNDATION && to.kind == LOC_COL_TAIL) {
            if (!move_foundation_to_column(&current_game.foundations[from.index],
                                           &current_game.columns[to.index])) {
                strcpy(message, "Illegal move.");
                return 1;
            }

            set_move_message();
            return 1;
        }

        if (from.kind == LOC_COL_TAIL && to.kind == LOC_COL_TAIL) {
            if (!move_bottom_card_to_column(&current_game.columns[from.index],
                                            &current_game.columns[to.index])) {
                strcpy(message, "Illegal move.");
                return 1;
            }

            set_move_message();
            return 1;
        }
        if (from.kind == LOC_COL_CARD && to.kind == LOC_COL_TAIL) {
            if (!move_stack_to_column(&current_game.columns[from.index],
                                      &current_game.columns[to.index],
                                      from.card)) {
                strcpy(message, "Illegal move.");
                return 1;
            }

            set_move_message();
            return 1;
        }

        strcpy(message, "Not implemented yet.");
        return 1;
    }

    strcpy(message, "Command not available yet.");

    return 1;
}

// tjekker om spillet er vundet
static int is_game_won(void) {
    int total = 0;
    int i;

    for (i = 0; i < FOUNDATIONS; i++) {
        total += list_length(current_game.foundations[i]);
    }

    return total == DECK_SIZE;
}

// besked efter et legal move
static void set_move_message(void) {
    if (is_game_won()) {
        strcpy(message, "You won.");
    } else {
        strcpy(message, "OK");
    }
}

#ifdef GAME_LOGIC_BACKEND
static void backend_format_card(Card card, char out[3]) {
    if (!card.face_up) {
        out[0] = 'X';
        out[1] = 'X';
    } else {
        out[0] = rank_to_char(card.rank);
        out[1] = suit_to_char(card.suit);
    }
    out[2] = '\0';
}

static void backend_print_pile(const char *prefix, int index, CardNode *pile) {
    CardNode *current = pile;
    int first = 1;

    printf("%s%d:", prefix, index + 1);
    while (current != NULL) {
        char token[3];

        backend_format_card(current->card, token);
        if (!first) {
            printf(",");
        }
        printf("%s", token);
        first = 0;
        current = current->next;
    }
    printf("\n");
}

static void backend_print_state(void) {
    int i;

    printf("PHASE:%s\n", current_phase == PHASE_PLAY ? "PLAY" : "STARTUP");
    for (i = 0; i < COLS; i++) {
        backend_print_pile("C", i, current_game.columns[i]);
    }
    for (i = 0; i < FOUNDATIONS; i++) {
        backend_print_pile("F", i, current_game.foundations[i]);
    }
}

static void backend_print_deck_view(void) {
    Gamestate view = {0};
    CardNode *current = current_deck;
    int row;
    int col;
    int i;

    for (row = 0; row < ROWS && current != NULL; row++) {
        for (col = 0; col < COLS && current != NULL; col++) {
            Card card = current->card;
            CardNode *node;

            card.face_up = true;
            node = node_create(card);
            if (node == NULL) {
                clear_tableau(&view);
                backend_print_state();
                return;
            }

            append(&view.columns[col], node);
            current = current->next;
        }
    }

    printf("PHASE:%s\n", current_phase == PHASE_PLAY ? "PLAY" : "STARTUP");
    for (i = 0; i < COLS; i++) {
        backend_print_pile("C", i, view.columns[i]);
    }
    for (i = 0; i < FOUNDATIONS; i++) {
        backend_print_pile("F", i, view.foundations[i]);
    }

    clear_tableau(&view);
}

static void backend_print_allowed_command(int *first, const char *command) {
    if (!*first) {
        printf(",");
    }
    printf("%s", command);
    *first = 0;
}

static void backend_print_column_card_command(int *first, int column, Card card, const char *target) {
    char token[3];
    char command[20];

    backend_format_card(card, token);
    snprintf(command, sizeof(command), "C%d:%s->%s", column + 1, token, target);
    backend_print_allowed_command(first, command);
}

static void backend_print_allowed_commands(void) {
    int first = 1;
    int source_index;
    int target_index;

    printf("A:");

    if (current_phase == PHASE_STARTUP) {
        backend_print_allowed_command(&first, "LD");
        if (deck_loaded && current_deck != NULL) {
            backend_print_allowed_command(&first, "SW");
            backend_print_allowed_command(&first, "SI");
            backend_print_allowed_command(&first, "SR");
            backend_print_allowed_command(&first, "SD");
            backend_print_allowed_command(&first, "P");
        }
        backend_print_allowed_command(&first, "QQ");
        printf("\n");
        return;
    }

    backend_print_allowed_command(&first, "Q");
    backend_print_allowed_command(&first, "QQ");

    for (source_index = 0; source_index < COLS; source_index++) {
        CardNode *source = current_game.columns[source_index];
        CardNode *current = source;

        while (current != NULL) {
            if (current->card.face_up) {
                for (target_index = 0; target_index < COLS; target_index++) {
                    char target[4];

                    if (source_index == target_index) {
                        continue;
                    }
                    if (!can_place_on_column(current->card, current_game.columns[target_index])) {
                        continue;
                    }

                    snprintf(target, sizeof(target), "C%d", target_index + 1);
                    backend_print_column_card_command(&first, source_index, current->card, target);
                }

                if (current->next == NULL) {
                    for (target_index = 0; target_index < FOUNDATIONS; target_index++) {
                        char target[4];

                        if (!can_move_to_foundation(current->card, current_game.foundations[target_index])) {
                            continue;
                        }

                        snprintf(target, sizeof(target), "F%d", target_index + 1);
                        backend_print_column_card_command(&first, source_index, current->card, target);
                    }
                }
            }
            current = current->next;
        }
    }

    for (source_index = 0; source_index < FOUNDATIONS; source_index++) {
        Card card;

        if (current_game.foundations[source_index] == NULL) {
            continue;
        }

        card = peek_tail(current_game.foundations[source_index]);
        for (target_index = 0; target_index < COLS; target_index++) {
            char command[12];

            if (!can_place_on_column(card, current_game.columns[target_index])) {
                continue;
            }

            snprintf(command, sizeof(command), "F%d->C%d", source_index + 1, target_index + 1);
            backend_print_allowed_command(&first, command);
        }
    }

    printf("\n");
}

static void backend_print_response(void) {
    int success = message[0] == '\0' || strcmp(message, "OK") == 0 || strcmp(message, "You won.") == 0;

    printf("STATUS:%s\n", success ? "OK" : "ERROR");
    printf("MESSAGE:%s\n", message);
    printf("LAST:%s\n", last_command);
    if (show_deck_view) {
        backend_print_deck_view();
        show_deck_view = 0;
    } else {
        backend_print_state();
    }
    backend_print_allowed_commands();
    printf("END\n");
    fflush(stdout);
}

static int game_logic_backend_run(void) {
    char input[140];
    char command[40];
    char arg[100];
    int running = 1;
    int parts;

    while (running && fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\n")] = '\0';
        parts = sscanf(input, "%39s %99s", command, arg);

        if (parts < 1) {
            last_command[0] = '\0';
            strcpy(message, "Command not available yet.");
            backend_print_response();
            continue;
        }

        if (strcmp(command, "STATE") == 0) {
            backend_print_response();
            continue;
        }

        snprintf(last_command, sizeof(last_command), "%s", input);
        if (parts == 1) {
            running = handle_command(command, NULL);
        } else {
            running = handle_command(command, arg);
        }

        backend_print_response();
    }

    clear_tableau(&current_game);
    clear_foundations(&current_game);
    free_list(&current_deck);
    return 0;
}

int main(void) {
    return game_logic_backend_run();
}
#endif
