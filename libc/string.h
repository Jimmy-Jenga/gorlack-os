#ifndef STRING_H
#define STRING_H

#include <stdint.h>

void int_to_ascii(int n, char str[]);
void hex_to_ascii(uint32_t n, char str[]);
void reverse(char s[]);
int strlen(char s[]);
void backspace(char s[]);
void append(char s[], char n);
int strcmp(char s1[], char s2[]);

#endif