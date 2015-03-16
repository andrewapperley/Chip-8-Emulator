//
//  cpu.h
//  Chip-8 Emulator
//
//  Created by Andrew Apperley on 2015-03-15.
//  Copyright (c) 2015 Andrew Apperley. All rights reserved.
//

#ifndef __Chip_8_Emulator__cpu__
#define __Chip_8_Emulator__cpu__

#include <stdio.h>
#include <stdbool.h>

extern bool draw;
extern bool shutDown;

void initialize();
void loadROM(const char *romName);
void runCycle();
void setKeys();
void drawGraphics();

#endif /* defined(__Chip_8_Emulator__cpu__) */