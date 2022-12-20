#ifndef PRINT_H
#define PRINT_H

#include <base.h>
#include <string.h>


void initialize_print_buffer(void *memory, usize size);
void print_flush(void);
void print(char const *c_string);
void print_n(char const *string, usize size);
void print_s(struct string s);
void print_f(char const *fmt, ...);


#endif // PRINT_H
