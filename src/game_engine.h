#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "card.h"

typedef enum {
    YUKON_PHASE_STARTUP,
    YUKON_PHASE_PLAY
} YukonPhase;

typedef struct {
    YukonPhase current_phase;
    Gamestate current_game;
    Gamestate display_game;
    CardNode *current_deck;
    int deck_loaded;
    int last_command_ok;
    char message[128];
} YukonGame;

void yukon_game_init(YukonGame *game);
void yukon_game_destroy(YukonGame *game);
int yukon_game_execute(YukonGame *game, const char *input);
const Gamestate *yukon_game_display_state(const YukonGame *game);
const char *yukon_game_message(const YukonGame *game);
int yukon_game_last_command_ok(const YukonGame *game);
YukonPhase yukon_game_phase(const YukonGame *game);

#endif
