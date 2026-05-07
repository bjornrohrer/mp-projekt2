#include <stdio.h>
#include <string.h>

#include "game_engine.h"
#include "text_ui.h"

int game_logic_run(void) {
    YukonGame game;
    char input[140];
    char last_command[140] = "";
    int running = 1;

    yukon_game_init(&game);

    while (running) {
        print_gamestate(yukon_game_display_state(&game));
        printf("\nLAST Command: %s\n", last_command);
        printf("Message: %s\n", yukon_game_message(&game));
        printf("INPUT > ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Input error\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';
        if (input[0] != '\0') {
            snprintf(last_command, sizeof(last_command), "%s", input);
        }

        running = yukon_game_execute(&game, input);
    }

    yukon_game_destroy(&game);
    return 0;
}
