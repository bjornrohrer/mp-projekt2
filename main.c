//
// Created by Bjørn Arthur Rohrer on 26/03/2026.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/card.h"

int load_deck(const char *filename, CardNode **deck);

int main(void) {
    CardNode *deck = NULL;
    char command[100];
    char filename[100];

    while (1) {
        printf("Enter command: ");

        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        if (sscanf(command, "LD %99s", filename) == 1) {
            if (load_deck(filename, &deck)) {
                printf("Deck loaded successfully.\n", filename);
            } else {
                printf("Deck not loaded successfully.\n", filename);
            }
        } else if (strcmp(command, "quit") == 0) {
            break;
        } else {
            printf("Unknown command.\n");
        }
    }
    return 0;
}