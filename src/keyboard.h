#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <stdint.h>

void handle_keyboard_input(uint8_t scancode);
void init_keyboard(void);

#endif
