#ifndef __STD_LIB_H__
#define __STD_LIB_H__

#include "std_type.h"

int div(int a, int b);
int mod(int a, int b);
void memcpy(byte* dst, byte* src, unsigned size);
unsigned strlen(char* str);
bool strcmp(char* str1, char* str2);
void strcpy(char* dst, char* src);
void clear(byte* buf, unsigned size);

#endif // __STD_LIB_H__