#ifndef CARD_H
#define CARD_H

#include <stdbool.h>

/* Card rank */
typedef enum {
    RANK_ACE = 1,
    RANK_TWO, RANK_THREE, RANK_FOUR, RANK_FIVE, RANK_SIX, RANK_SEVEN,
    RANK_EIGHT, RANK_NINE, RANK_TEN, RANK_JACK, RANK_QUEEN, RANK_KING
} Rank;

/* Card suits */
typedef enum {
    SUIT_CLUBS = 0,
    SUIT_SPADE,
    SUIT_DIAMOND,
    SUIT_HEART
} Suit;

typedef struct {
    Rank rank;
    Suit suit;
    bool face_up;
} Card;

typedef struct CardNode {
    Card card;
    struct CardNode *next;
} CardNode;

char rank_to_char(Rank rank);
char suit_to_char(Suit suit);
bool card_from_string(const char *s, Card *out);

#endif
fuck nicolas