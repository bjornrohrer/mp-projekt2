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
        if (digit > '7') {
            return 0;
        }
        out->index = digit - '1';

        if (length == 2) {
            out->kind = LOC_COL_TAIL;
            return 1;
        }
        if (length == 4) {
            if (!card_from_string(s + 2, &out->card)) {
                return 0;
            }
            out->kind = LOC_COL_CARD;
            return 1;
        }
        return 0;
    }

    if (prefix == 'F') {
        if (digit > '4') {
            return 0;
        }
        if (length != 2) {
            return 0;
        }
        out->index = digit - '1';
        out->kind = LOC_FOUNDATION;
        return 1;
    }

    return 0;
}

int parse_move(const char *input, Location *from, Location *to) {
    if (input == NULL || from == NULL || to == NULL) {
        return 0;
    }

    const char *arrow = strstr(input, "->");
    if (arrow == NULL) {
        return 0;
    }

    size_t from_len = (size_t) (arrow - input);
    if (from_len < 2 || from_len > 4) {
        return 0;
    }

    char from_buf[8];
    memcpy(from_buf, input, from_len);
    from_buf[from_len] = '\0';

    const char *to_part = arrow + 2;
    size_t to_len = strlen(to_part);
    if (to_len < 2 || to_len > 4) {
        return 0;
    }

    if (!parse_location(from_buf, from)) {
        return 0;
    }
    if (!parse_location(to_part, to)) {
        return 0;
    }
    return 1;
}
