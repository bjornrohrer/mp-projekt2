#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "deck.h"
#include "game_engine.h"
#include "input.h"
#include "list.h"

#define ROWS 11
#define COLS 7
#define FOUNDATIONS 4
#define DECK_SIZE 52

static int first_col_in_row(int row);
static void clear_tableau(Gamestate *game);
static void clear_foundations(Gamestate *game);
static void clear_gamestate(Gamestate *game);
static void set_message(YukonGame *game, const char *message, int ok);
static int build_deck_display(YukonGame *game, int reveal_cards);
static int deal_from_current_deck(YukonGame *game);
static bool can_move_to_foundation(Card card, CardNode *foundation);
static int move_card_to_foundation(CardNode **source, CardNode **foundation);
static int move_selected_card_to_foundation(CardNode **source, CardNode **foundation, Card selected);
static int move_foundation_to_column(CardNode **foundation, CardNode **destination);
static void flip_new_bottom_card(CardNode **column);
static bool can_place_on_column(Card card, CardNode *destination);
static int move_bottom_card_to_column(CardNode **source, CardNode **destination);
static int move_stack_to_column(CardNode **source, CardNode **destination, Card selected);
static int is_game_won(const YukonGame *game);
static void set_move_message(YukonGame *game);
static int is_startup_command(const char *command);

void yukon_game_init(YukonGame *game) {
    if (game == NULL) {
        return;
    }

    memset(game, 0, sizeof(*game));
    game->current_phase = YUKON_PHASE_STARTUP;
    game->last_command_ok = 1;
}

void yukon_game_destroy(YukonGame *game) {
    if (game == NULL) {
        return;
    }

    clear_gamestate(&game->current_game);
    clear_gamestate(&game->display_game);
    free_list(&game->current_deck);
    game->deck_loaded = 0;
    game->current_phase = YUKON_PHASE_STARTUP;
}

const Gamestate *yukon_game_display_state(const YukonGame *game) {
    if (game == NULL) {
        return NULL;
    }

    if (game->current_phase == YUKON_PHASE_PLAY) {
        return &game->current_game;
    }

    return &game->display_game;
}

const char *yukon_game_message(const YukonGame *game) {
    if (game == NULL) {
        return "";
    }

    return game->message;
}

int yukon_game_last_command_ok(const YukonGame *game) {
    if (game == NULL) {
        return 0;
    }

    return game->last_command_ok;
}

YukonPhase yukon_game_phase(const YukonGame *game) {
    if (game == NULL) {
        return YUKON_PHASE_STARTUP;
    }

    return game->current_phase;
}

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

static void clear_gamestate(Gamestate *game) {
    clear_tableau(game);
    clear_foundations(game);
}

static void set_message(YukonGame *game, const char *message, int ok) {
    snprintf(game->message, sizeof(game->message), "%s", message);
    game->last_command_ok = ok;
}

static int build_deck_display(YukonGame *game, int reveal_cards) {
    CardNode *current;
    Card card;
    int col;
    int index = 0;

    if (game == NULL || !game->deck_loaded || game->current_deck == NULL) {
        return 0;
    }

    if (list_length(game->current_deck) != DECK_SIZE) {
        return 0;
    }

    clear_gamestate(&game->display_game);
    current = game->current_deck;

    while (current != NULL) {
        card = current->card;
        card.face_up = reveal_cards;
        col = index % COLS;

        CardNode *node = node_create(card);
        if (node == NULL) {
            clear_gamestate(&game->display_game);
            return 0;
        }

        append(&game->display_game.columns[col], node);
        current = current->next;
        index++;
    }

    return index == DECK_SIZE;
}

static int deal_from_current_deck(YukonGame *game) {
    int row;
    int col;
    int start_col;
    int index = 0;
    int count_in_col[COLS] = {0};
    CardNode *current;
    Card card;

    if (!game->deck_loaded || game->current_deck == NULL) {
        return 0;
    }

    if (list_length(game->current_deck) != DECK_SIZE) {
        return 0;
    }

    clear_gamestate(&game->current_game);
    current = game->current_deck;

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

            append(&game->current_game.columns[col], node);
            current = current->next;
            index++;
        }
    }

    return index == DECK_SIZE;
}

static bool can_move_to_foundation(Card card, CardNode *foundation) {
    if (foundation == NULL) {
        return card.rank == RANK_ACE;
    }

    Card top = peek_tail(foundation);
    return card.rank == top.rank + 1 && card.suit == top.suit;
}

static int move_card_to_foundation(CardNode **source, CardNode **foundation) {
    Card top;
    CardNode *node;

    if (*source == NULL) {
        return 0;
    }

    top = peek_tail(*source);
    if (!top.face_up) {
        return 0;
    }

    if (!can_move_to_foundation(top, *foundation)) {
        return 0;
    }

    node = node_create(top);
    if (node == NULL) {
        return 0;
    }

    pop_tail(source);
    flip_new_bottom_card(source);
    append(foundation, node);
    return 1;
}

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

    if (current == NULL || current->next != NULL) {
        return 0;
    }

    return move_card_to_foundation(source, foundation);
}

static int move_foundation_to_column(CardNode **foundation, CardNode **destination) {
    Card card;
    CardNode *node;

    if (*foundation == NULL) {
        return 0;
    }

    card = peek_tail(*foundation);
    if (!can_place_on_column(card, *destination)) {
        return 0;
    }

    node = node_create(card);
    if (node == NULL) {
        return 0;
    }

    pop_tail(foundation);
    append(destination, node);
    return 1;
}

static void flip_new_bottom_card(CardNode **column) {
    CardNode *tail = tail_node(*column);

    if (tail != NULL && !tail->card.face_up) {
        tail->card.face_up = true;
    }
}

static bool can_place_on_column(Card card, CardNode *destination) {
    Card bottom;

    if (destination == NULL) {
        return card.rank == RANK_KING;
    }

    bottom = peek_tail(destination);
    return bottom.rank == card.rank + 1 && bottom.suit != card.suit;
}

static int move_bottom_card_to_column(CardNode **source, CardNode **destination) {
    Card card;
    CardNode *node;

    if (*source == NULL) {
        return 0;
    }

    card = peek_tail(*source);
    if (!card.face_up) {
        return 0;
    }

    if (!can_place_on_column(card, *destination)) {
        return 0;
    }

    node = node_create(card);
    if (node == NULL) {
        return 0;
    }

    pop_tail(source);
    flip_new_bottom_card(source);
    append(destination, node);
    return 1;
}

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

    if (current == NULL || !current->card.face_up) {
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

static int is_game_won(const YukonGame *game) {
    int total = 0;
    int i;

    for (i = 0; i < FOUNDATIONS; i++) {
        total += list_length(game->current_game.foundations[i]);
    }

    return total == DECK_SIZE;
}

static void set_move_message(YukonGame *game) {
    if (is_game_won(game)) {
        set_message(game, "You won.", 1);
    } else {
        set_message(game, "OK", 1);
    }
}

static int is_startup_command(const char *command) {
    if (strcmp(command, "LD") == 0) return 1;
    if (strcmp(command, "SW") == 0) return 1;
    if (strcmp(command, "SI") == 0) return 1;
    if (strcmp(command, "SR") == 0) return 1;
    if (strcmp(command, "SD") == 0) return 1;
    return 0;
}

int yukon_game_execute(YukonGame *game, const char *input) {
    char command[40];
    char arg[100];
    int parts;

    if (game == NULL || input == NULL) {
        return 1;
    }

    parts = sscanf(input, "%39s %99s", command, arg);
    if (parts < 1) {
        set_message(game, "Command not available yet.", 0);
        return 1;
    }

    if (strcmp(command, "QQ") == 0) {
        set_message(game, "OK", 1);
        return 0;
    }

    if (game->current_phase == YUKON_PHASE_PLAY && is_startup_command(command)) {
        set_message(game, "Command not available in the PLAY phase.", 0);
        return 1;
    }

    if (strcmp(command, "LD") == 0) {
        clear_gamestate(&game->current_game);
        clear_gamestate(&game->display_game);
        free_list(&game->current_deck);
        game->current_phase = YUKON_PHASE_STARTUP;

        if (parts == 1) {
            if (!generate_unshuffled_deck(&game->current_deck)) {
                game->deck_loaded = 0;
                set_message(game, "Could not load deck.", 0);
                return 1;
            }
        } else {
            if (!load_deck(arg, &game->current_deck)) {
                game->deck_loaded = 0;
                set_message(game, "Could not load deck.", 0);
                return 1;
            }
        }

        game->deck_loaded = 1;
        if (!build_deck_display(game, 0)) {
            set_message(game, "Could not show deck.", 0);
            return 1;
        }

        set_message(game, "OK", 1);
        return 1;
    }

    if (strcmp(command, "SW") == 0) {
        if (!game->deck_loaded || game->current_deck == NULL) {
            set_message(game, "No deck loaded.", 0);
            return 1;
        }

        if (list_length(game->current_deck) != DECK_SIZE) {
            set_message(game, "No deck loaded.", 0);
            return 1;
        }

        if (!build_deck_display(game, 1)) {
            set_message(game, "Could not show deck.", 0);
            return 1;
        }

        set_message(game, "OK", 1);
        return 1;
    }

    if (strcmp(command, "SD") == 0) {
        const char *filename = parts == 1 ? "cards.txt" : arg;

        if (!game->deck_loaded || game->current_deck == NULL) {
            set_message(game, "No deck loaded.", 0);
            return 1;
        }

        if (!save_deck(filename, game->current_deck)) {
            set_message(game, "Could not save deck.", 0);
            return 1;
        }

        set_message(game, "OK", 1);
        return 1;
    }

    if (strcmp(command, "SI") == 0) {
        int split;
        int length;

        if (!game->deck_loaded || game->current_deck == NULL) {
            set_message(game, "No deck loaded.", 0);
            return 1;
        }

        length = list_length(game->current_deck);
        if (parts == 1) {
            srand((unsigned int) time(NULL));
            split = (rand() % (length - 1)) + 1;
        } else {
            if (sscanf(arg, "%d", &split) != 1) {
                set_message(game, "Invalid split.", 0);
                return 1;
            }
        }

        if (!shuffle_interleave_split(&game->current_deck, split)) {
            set_message(game, "Invalid split.", 0);
            return 1;
        }

        set_message(game, "OK", 1);
        return 1;
    }

    if (strcmp(command, "SR") == 0) {
        if (!game->deck_loaded || game->current_deck == NULL) {
            set_message(game, "No deck loaded.", 0);
            return 1;
        }

        shuffle_random(&game->current_deck);
        set_message(game, "OK", 1);
        return 1;
    }

    if (strcmp(command, "P") == 0) {
        if (game->current_phase == YUKON_PHASE_PLAY) {
            set_message(game, "Command not available in the PLAY phase.", 0);
            return 1;
        }

        if (!game->deck_loaded) {
            set_message(game, "No deck loaded.", 0);
            return 1;
        }

        clear_gamestate(&game->display_game);
        if (!deal_from_current_deck(game)) {
            set_message(game, "Error while dealing cards", 0);
            return 1;
        }

        game->current_phase = YUKON_PHASE_PLAY;
        set_message(game, "OK", 1);
        return 1;
    }

    if (strcmp(command, "Q") == 0) {
        if (game->current_phase == YUKON_PHASE_PLAY) {
            clear_gamestate(&game->current_game);
            clear_gamestate(&game->display_game);
            game->current_phase = YUKON_PHASE_STARTUP;
            set_message(game, "OK", 1);
        } else {
            set_message(game, "Command not available in the STARTUP phase.", 0);
        }
        return 1;
    }

    if (strstr(command, "->") != NULL) {
        Location from;
        Location to;

        if (game->current_phase != YUKON_PHASE_PLAY) {
            set_message(game, "Command not available in the STARTUP phase.", 0);
            return 1;
        }

        if (!parse_move(command, &from, &to)) {
            set_message(game, "Malformed move.", 0);
            return 1;
        }

        if (from.kind == LOC_COL_TAIL && to.kind == LOC_FOUNDATION) {
            if (!move_card_to_foundation(&game->current_game.columns[from.index],
                                         &game->current_game.foundations[to.index])) {
                set_message(game, "Illegal move.", 0);
                return 1;
            }

            set_move_message(game);
            return 1;
        }

        if (from.kind == LOC_COL_CARD && to.kind == LOC_FOUNDATION) {
            if (!move_selected_card_to_foundation(&game->current_game.columns[from.index],
                                                  &game->current_game.foundations[to.index],
                                                  from.card)) {
                set_message(game, "Illegal move.", 0);
                return 1;
            }

            set_move_message(game);
            return 1;
        }

        if (from.kind == LOC_FOUNDATION && to.kind == LOC_COL_TAIL) {
            if (!move_foundation_to_column(&game->current_game.foundations[from.index],
                                           &game->current_game.columns[to.index])) {
                set_message(game, "Illegal move.", 0);
                return 1;
            }

            set_move_message(game);
            return 1;
        }

        if (from.kind == LOC_COL_TAIL && to.kind == LOC_COL_TAIL) {
            if (!move_bottom_card_to_column(&game->current_game.columns[from.index],
                                            &game->current_game.columns[to.index])) {
                set_message(game, "Illegal move.", 0);
                return 1;
            }

            set_move_message(game);
            return 1;
        }

        if (from.kind == LOC_COL_CARD && to.kind == LOC_COL_TAIL) {
            if (!move_stack_to_column(&game->current_game.columns[from.index],
                                      &game->current_game.columns[to.index],
                                      from.card)) {
                set_message(game, "Illegal move.", 0);
                return 1;
            }

            set_move_message(game);
            return 1;
        }

        set_message(game, "Not implemented yet.", 0);
        return 1;
    }

    set_message(game, "Command not available yet.", 0);
    return 1;
}
