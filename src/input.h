#ifndef INPUT_H
#define INPUT_H
#include "card.h"

typedef enum {
    LOC_COL_tail,
    LOC_col_CARD,
    LOC_FOUNDATION
} locKIND;

typedef struct {
    locKIND kind;
    int index;
    Card card;
} Location;

int parse_location(const char *s, Location *out);

#endif
