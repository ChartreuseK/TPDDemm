/*
 * Hayden Kroepfl - 2017
 */
#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

int serial_init(void);
void serial_close(void);
int serial_read(uint8_t *buf, int len);
uint8_t serial_read_byte(void);
void serial_write_byte(uint8_t byte);
int serial_write(uint8_t *buf, int len);

#endif
