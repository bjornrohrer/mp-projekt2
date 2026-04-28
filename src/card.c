#include <ctype.h>
#include <string.h>

#include "card.h"

char rank_to_char(Rank rank) {
    switch (rank) {
        case RANK_ACE:   return 'A';
        case RANK_TWO:   return '2';
        case RANK_THREE: return '3';
        case RANK_FOUR:  return '4';
        case RANK_FIVE:  return '5';
        case RANK_SIX:   return '6';
        case RANK_SEVEN: return '7';
        case RANK_EIGHT: return '8';
        case RANK_NINE:  return '9';
        case RANK_TEN:   return 'T';
        case RANK_JACK:  return 'J';
        case RANK_QUEEN: return 'Q';
        case RANK_KING:  return 'K';
        default:         return '?';
    }
}

char suit_to_char(Suit suit) {
    switch (suit) {
        case SUIT_CLUBS:   return 'C';
        case SUIT_SPADE:   return 'S';
        case SUIT_DIAMOND: return 'D';
        case SUIT_HEART:   return 'H';
        default:           return '?';
    }
}

bool card_from_string(const char *s, Card *out) {
    if (s == NULL || out == NULL) {
        return false;
    }
    if (strlen(s) != 2) {
        return false;
    }

    char r = (char) toupper((unsigned char) s[0]);
    char u = (char) toupper((unsigned char) s[1]);

    switch (r) {
        case 'A': out->rank = RANK_ACE;   break;
        case '2': out->rank = RANK_TWO;   break;
        case '3': out->rank = RANK_THREE; break;
        case '4': out->rank = RANK_FOUR;  break;
        case '5': out->rank = RANK_FIVE;  break;
        case '6': out->rank = RANK_SIX;   break;
        case '7': out->rank = RANK_SEVEN; break;
        case '8': out->rank = RANK_EIGHT; break;
        case '9': out->rank = RANK_NINE;  break;
        case 'T': out->rank = RANK_TEN;   break;
        case 'J': out->rank = RANK_JACK;  break;
        case 'Q': out->rank = RANK_QUEEN; break;
        case 'K': out->rank = RANK_KING;  break;
        default: return false;
    }

    switch (u) {
        case 'C': out->suit = SUIT_CLUBS;   break;
        case 'S': out->suit = SUIT_SPADE;   break;
        case 'D': out->suit = SUIT_DIAMOND; break;
        case 'H': out->suit = SUIT_HEART;   break;
        default: return false;
    }

    out->face_up = false;
    return true;
}
