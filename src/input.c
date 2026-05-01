#include <string.h>
#include <ctype.h>
#include "input.h"
#include "card.h"

int parse_location(const char *s, Location *out) {
    if (s == NULL || out == NULL) {
        return 0;
    }

    size_t length = strlen(s);
    if (length < 2) {
        return 0;
    }

    char prefix = (char) toupper((unsigned char) s[0]);
    char digit = s[1];

    if (digit < '1' || digit > '9') {
        return 0;
    }

    if (prefix == 'C') {
        if (digit > '7') return 0;
        out->index = digit - '1';

        if (length == 2) {
            out->kind = LOC_COL_TAIL;
            return 1;
        }
        if (length == 4) {
            if (!card_from_string(s + 2, &out->card)) return 0;
            out->kind = LOC_COL_CARD;
            return 1;
        }
        return 0;
    }

    if (prefix == 'F') {
        if (digit > '4') return 0;
        if (length != 2) return 0;

        out->index = digit - '1';
        out->kind = LOC_FOUNDATION;
        return 1;
    }

    return 0;
}
