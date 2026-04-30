#ifndef DECK_H
#define DECK_H

#include "card.h"

int load_deck(const char *filename, CardNode **deck);
int generate_unshuffled_deck(CardNode **deck);
void show_deck(CardNode *deck);
int save_deck(const char *filename, CardNode *deck);
void shuffle_interleave_from_card(CardNode **deck, Rank rank, Suit suit);
void shuffle_random(CardNode **deck);

#endif
