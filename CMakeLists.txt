cmake_minimum_required(VERSION 4.1)
project(mp_projekt2 C)

set(CMAKE_C_STANDARD 11)

add_executable(mp_projekt2
        main.c
        src/card.h
        src/list.c
        src/deck.c
        src/input.c
        src/game_logic.c
        src/main_text.c
        src/text_ui.c)
