#include <stdint.h>
#include "page.h"
#include "rprintf.h"

extern int putc(int data);

int count_pages(struct ppage *list) {
    int count = 0;
    struct ppage *current = list;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

void print_page_list(const char *name, struct ppage *list) {
    int count = count_pages(list);
    esp_printf(putc, "%s: %d pages\r\n", name, count);
    if (list != NULL && count <= 5) {
        struct ppage *current = list;
        while (current != NULL) {
            esp_printf(putc, "  Page at 0x%x\r\n", (unsigned int)current->physical_addr);
            current = current->next;
        }
    }
}

void test_page_allocator(void) {
    struct ppage *allocated1 = NULL;
    struct ppage *allocated2 = NULL;
    struct ppage *allocated3 = NULL;
    esp_printf(putc, "\r\n=== Page Frame Allocator Tests ===\r\n\r\n");
    esp_printf(putc, "Test 1: Initializing...\r\n");
    init_pfa_list();
    print_page_list("Free pages after init", get_free_list());
    esp_printf(putc, "\r\nTest 2: Allocating 1 page...\r\n");
    allocated1 = allocate_physical_pages(1);
    print_page_list("Allocated", allocated1);
    print_page_list("Free remaining", get_free_list());
    esp_printf(putc, "\r\nTest 3: Allocating 5 pages...\r\n");
    allocated2 = allocate_physical_pages(5);
    print_page_list("Allocated", allocated2);
    print_page_list("Free remaining", get_free_list());
    esp_printf(putc, "\r\nTest 4: Allocating 10 pages...\r\n");
    allocated3 = allocate_physical_pages(10);
    print_page_list("Allocated", allocated3);
    print_page_list("Free remaining", get_free_list());
    esp_printf(putc, "\r\nTest 5: Freeing 1 page...\r\n");
    free_physical_pages(allocated1);
    allocated1 = NULL;
    print_page_list("Free after freeing 1", get_free_list());
    esp_printf(putc, "\r\nTest 6: Freeing 5 pages...\r\n");
    free_physical_pages(allocated2);
    allocated2 = NULL;
    print_page_list("Free after freeing 5", get_free_list());
    esp_printf(putc, "\r\nTest 7: Freeing 10 pages...\r\n");
    free_physical_pages(allocated3);
    allocated3 = NULL;
    print_page_list("Free after freeing 10", get_free_list());
    esp_printf(putc, "\r\n=== All Tests Complete ===\r\n\r\n");
}
