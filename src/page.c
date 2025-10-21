#include "page.h"
#include <stddef.h>

#define PAGE_SIZE 0x200000
#define PHYSICAL_MEMORY_START 0x00000000

struct ppage physical_page_array[128];
static struct ppage *free_pp_list = NULL;

void init_pfa_list(void) {
    int i;
    physical_page_array[0].next = NULL;
    physical_page_array[0].prev = NULL;
    physical_page_array[0].physical_addr = (void*)(PHYSICAL_MEMORY_START);
    free_pp_list = &physical_page_array[0];
    for (i = 1; i < 128; i++) {
        physical_page_array[i].physical_addr = (void*)(PHYSICAL_MEMORY_START + (i * PAGE_SIZE));
        physical_page_array[i].prev = &physical_page_array[i - 1];
        physical_page_array[i].next = NULL;
        physical_page_array[i - 1].next = &physical_page_array[i];
    }
}

struct ppage *allocate_physical_pages(unsigned int npages) {
    struct ppage *allocated_list = NULL;
    struct ppage *current = NULL;
    struct ppage *temp = NULL;
    unsigned int i;
    if (free_pp_list == NULL || npages == 0) {
        return NULL;
    }
    unsigned int available = 0;
    temp = free_pp_list;
    while (temp != NULL) {
        available++;
        temp = temp->next;
    }
    if (available < npages) {
        return NULL;
    }
    for (i = 0; i < npages; i++) {
        if (free_pp_list == NULL) {
            return allocated_list;
        }
        current = free_pp_list;
        free_pp_list = free_pp_list->next;
        if (free_pp_list != NULL) {
            free_pp_list->prev = NULL;
        }
        if (allocated_list == NULL) {
            allocated_list = current;
            current->next = NULL;
            current->prev = NULL;
        } else {
            temp = allocated_list;
            while (temp->next != NULL) {
                temp = temp->next;
            }
            temp->next = current;
            current->prev = temp;
            current->next = NULL;
        }
    }
    return allocated_list;
}

void free_physical_pages(struct ppage *ppage_list) {
    struct ppage *tail = NULL;
    if (ppage_list == NULL) {
        return;
    }
    tail = ppage_list;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    if (free_pp_list != NULL) {
        tail->next = free_pp_list;
        free_pp_list->prev = tail;
    }
    ppage_list->prev = NULL;
    free_pp_list = ppage_list;
}

struct ppage *get_free_list(void) {
    return free_pp_list;
}
