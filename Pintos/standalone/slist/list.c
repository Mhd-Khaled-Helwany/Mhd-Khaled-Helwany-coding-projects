#include "list.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void append(struct list_item *first, int x){
    struct list_item* current = first;
    do {    
        if (current->next == NULL){
            struct list_item* new = malloc(sizeof(struct list_item));
            new->value = x;
            new->next = NULL;
            current->next = new;
            return;
        }
        else {
            current = current->next;
        }
    }while (current);
}

void prepend(struct list_item *first, int x){
    struct list_item* new = malloc(sizeof(struct list_item));
    new->value = x;
    new->next = first->next;
    first->next = new;
}

void input_sorted(struct list_item *first, int x){
    struct list_item* new = malloc(sizeof(struct list_item));
    new->value = x;
    struct list_item* current = first;
    while (current->next && current->next->value < x){
        current = current->next;
    }
    new->next = current->next;
    current->next = new;
    
}

void print(struct list_item *first){
    struct list_item* current = first->next; //ska vi ha root värden med?
    while (current){
        printf("%d\n", current->value);
		current = current->next;
    }
}

void clear(struct list_item *first){
    struct list_item* current = first->next;
    while (current){
        struct list_item* item = current;
        current = current->next;
        free (item);
    }
    first->next = NULL;
}

/* Implement the functions here */
