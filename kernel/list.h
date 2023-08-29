#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct list {
    struct list *prev;
    struct list *next;
} list;

typedef list list_t;
typedef list list_elem_t;

#define LIST_INIT(list)                                                                     \
({  (list)->next = (list);                                                                  \
    (list)->prev = (list);                                                                  \
})

#define LIST_CONTAINER(elem, container, field)                                              \
    (elem == NULL ? NULL :(container *) ((uintptr_t) (elem) - offsetof(container, field)))

#define LIST_FOR_EACH(elem, list, container, field)                                         \
    for (container *elem = LIST_CONTAINER((list)->next, container, field);                  \
        &elem->field != (list);                                                             \
        elem = LIST_CONTAINER(elem->field.next, container, field))

void list_push_back(list_t *list, list_elem_t *elem);
list_elem_t * list_head(list_t *list);
list_elem_t * list_tail(list_t *list);
list_elem_t * list_pop_tail(list_t *list);