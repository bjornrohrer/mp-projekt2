#include <stdio.h>
#include <string.h>

#include "card.h"
#include "game_engine.h"
#include "list.h"

#define COLS 7
#define FOUNDATIONS 4

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

static void print_state(const Gamestate *state) {
    int i;

    if (state == NULL) {
        return;
    }

    for (i = 0; i < COLS; i++) {
        print_line_for_pile("C", i, state->columns[i]);
    }

    for (i = 0; i < FOUNDATIONS; i++) {
        print_line_for_pile("F", i, state->foundations[i]);
    }
}

static void respond(const YukonGame *game) {
    const char *message = yukon_game_message(game);

    printf("%s", yukon_game_last_command_ok(game) ? "OK" : "ERROR");
    if (message[0] != '\0' && strcmp(message, "OK") != 0) {
        printf(" %s", message);
    }
    printf("\n");

    print_state(yukon_game_display_state(game));
    printf("END\n");
    fflush(stdout);
}

int main(void) {
    YukonGame game;
    char input[256];
    int running = 1;

    yukon_game_init(&game);

    while (running && fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\n")] = '\0';
        running = yukon_game_execute(&game, input);
        respond(&game);
    }

    yukon_game_destroy(&game);
    return 0;
}
