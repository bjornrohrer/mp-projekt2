//
// Created by Sebastian Hee Jensen on 23/04/2026.
//
#ifndef LIST_H
#define LIST_H

#include "card.h"

CardNode *node_create(Card card);
void prepend(CardNode **head, CardNode *node);
void append(CardNode **head, CardNode *node);
Card pop_head(CardNode **head);
Card pop_tail(CardNode **head);
Card peek_head(CardNode *head);
Card peek_tail(CardNode *head);
int list_length(CardNode *head);
Card specific_node(CardNode *head, int index);
Card find_node(CardNode *head, Rank rank, Suit suit);
CardNode *split_list(CardNode **head, Rank rank, Suit suit);
void append_sublist(CardNode **head, CardNode *sublist);
void free_list(CardNode **head);

#endif