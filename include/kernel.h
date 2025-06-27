#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "std_type.h"

void readSector(byte* buf, int sector);
void writeSector(byte* buf, int sector);
void clearScreen(void);
void printString(char* str);
void readString(char* buf);

#endif // __KERNEL_H__