#include "list.h"

static void list_insert(list_elem_t *prev, list_elem_t *next, list_elem_t *new) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}

void list_push_back(list *list, list_elem_t *elem) {
    list_insert(list->prev, list, elem);
}