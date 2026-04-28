#ifndef TEXT_UI_H
#define TEXT_UI_H

#include "card.h"

void print_gamestate(const Gamestate *gs);
void print_deck(CardNode *deck);
void print_message(const char *message);

#endif
