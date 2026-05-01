#ifndef INPUT_H
#define INPUT_H
#include "card.h"

typedef enum {
    LOC_COL_TAIL,
    LOC_COL_CARD,
    LOC_FOUNDATION
} locKind;

typedef struct {
    locKind kind;
    int index;
    Card card;
} Location;

int parse_location(const char *s, Location *out);

#endif
