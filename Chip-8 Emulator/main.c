//
//  main.c
//  Chip-8 Emulator
//
//  Created by Andrew Apperley on 2015-03-15.
//  Copyright (c) 2015 Andrew Apperley. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

int main(int argc, const char * argv[]) {
    
    char *romName = "invaders.c8";
//    char *fullPath = (char *)malloc(sizeof(argv[0])+sizeof(romName));
//    strcpy(fullPath, argv[0]);
//    strcat(fullPath, romName);
    initialize();
    loadROM(romName);
    
    while (!shutDown) {
        runCycle();
        
        if (draw) {
            drawGraphics();
        }
        
        setKeys();
    }
    
    return 0;
}