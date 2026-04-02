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



#endif
