//
//  cpu.c
//  Chip-8 Emulator
//
//  Created by Andrew Apperley on 2015-03-15.
//  Copyright (c) 2015 Andrew Apperley. All rights reserved.
//

#include "cpu.h"
#include <string.h>
#include <stdlib.h>

bool draw = false;
bool shutDown = false;

unsigned short opcode;
unsigned char memory[4096];
unsigned char V[16];
unsigned short I;
unsigned short pc;
unsigned char gfx[2048];//64 * 32
unsigned char delayTimer;
unsigned char soundTimer;
unsigned short stack[16];
unsigned short sp;
unsigned char key[16];

SDL_Renderer *screenRenderer;

unsigned char chip8_fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

void initialize() {
    pc = 0x200;
    opcode = 0;
    I = 0;
    sp = 0;
    
    delayTimer = 0;
    soundTimer = 0;
    
    memset(gfx, 0, sizeof(gfx));
    memset(V, 0, sizeof(V));
    memset(stack, 0, sizeof(stack));
    memset(memory, 0, sizeof(memory));
    memset(key, 0, sizeof(key));
    
    for(int i = 0; i < sizeof(chip8_fontset); ++i) {
        memory[i] = chip8_fontset[i];
    }
    
}

void initializeGraphics() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error\n");
        exit(-1);
    } else {
        printf("SDL_Init Success\n");
        
        SDL_Window *screen;
        SDL_CreateWindowAndRenderer(640, 320, SDL_WINDOW_OPENGL, &screen, &screenRenderer);
        SDL_SetRenderDrawColor(screenRenderer, 0, 0, 0, 255);
        SDL_RenderClear(screenRenderer);
        SDL_RenderPresent(screenRenderer);
    }
}

void loadROM(const char *romName) {
    char *buffer;
    unsigned long fileLen;
    
    FILE *rom = fopen(romName, "rb");
    
    fseek(rom, 0, SEEK_END);
    fileLen = ftell(rom);
    fseek(rom, 0, SEEK_SET);
    
    buffer = (char *)malloc(fileLen+1);
    fread(buffer, fileLen, 1, rom);
    fclose(rom);
    
    for (int i = 0; i < fileLen; ++i) {
        memory[i + 512] = buffer[i];
    }
    free(buffer);
}

void runCycle() {
//    Fetch opcode
    opcode = memory[pc] << 8 | memory[pc + 1];
//    Decode opcode
    switch (opcode & 0xF000) {
            
        case 0x0000:
            switch(opcode & 0x000F) {
                    
            case 0x0000: // Clears the screen
                    memset(gfx, 0, sizeof(gfx));
                    draw = true;
                break;
                
            case 0x000E: // Returns from subroutine
                    sp--;
                    pc = stack[sp];
                    pc += 2;
                break;
                
            default:
                printf ("Unknown opcode [0x0000]: 0x%X\n", opcode);          
        }
            break;
            
        case 0x1000: // Jumps to address NNN.
            pc = opcode & 0x0FFF;
            break;
            
        case 0x2000: // Calls subroutine at NNN.
            stack[sp] = pc;
            ++sp;
            pc = opcode & 0x0FFF;
            break;
            
        case 0x3000: // Skips the next instruction if VX equals NN.
            if (V[(opcode & 0x0F00 >> 8)] == (opcode & 0x00FF)) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
            
        case 0x4000: // Skips the next instruction if VX doesn't equal NN.
            if (V[(opcode * 0x0F00 >> 8)] != (opcode & 0x00FF)) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
            
        case 0x5000: // Skips the next instruction if VX equals VY.
            if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 8]) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
            
        case 0x6000: // Sets VX to NN.
            V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
            pc += 2;
            break;
            
        case 0x7000: // Adds NN to VX.
            V[(opcode & 0x0F00) >> 8] = (V[(opcode & 0x0F00) >> 8] + (opcode & 0x00FF)) & 0xFF;
            pc += 2;
            break;
            
        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000: // Sets VX to the value of VY.
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                    
                case 0x0001: // Sets VX to VX or VY.
                    V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                    
                case 0x0002: // Sets VX to VX and VY.
                    V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                    
                case 0x0003: // Sets VX to VX xor VY.
                    V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                    
                case 0x0004: // Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
                    if (V[(opcode & 0x00F0) >> 4] > (0xFF/*255*/ - V[(opcode & 0x0F00) >> 8])) {
                        V[0xF/*15*/] = 1;
                    } else {
                        V[0xF/*15*/] = 0;
                    }
                    V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                    
                case 0x0005: // VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
                    if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4]) {
                        V[0xF/*15*/] = 1;
                    } else {
                        V[0xF/*15*/] = 0;
                    }
                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                    
                case 0x0006: // Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift.
                    V[0xF/*15*/] = (V[(opcode & 0x0F00) >> 8] & 0x1);
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    pc += 2;
                    break;
                    
                case 0x0007: // Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
                    if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) {
                        V[0xF/*15*/] = 1;
                    } else {
                        V[0xF/*15*/] = 0;
                    }
                    V[(opcode & 0x00F0) >> 4] -= V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                    
                case 0x000E: // Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift.
                    V[0xF/*15*/] = (V[(opcode & 0x0F00) >> 8] >> 7);
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    pc += 2;
                    break;
                    
                default:
                    printf("Unknown opcode: 0x%X\n", opcode);
                    break;
            }
            break;
            
        case 0x9000: // Skips the next instruction if VX doesn't equal VY.
            if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 8]) {
                pc += 4;
            } else {
                pc += 2;
            }

            break;
            
        case 0xA000: // Sets I to the address NNN
            I = (opcode & 0x0FFF);
            pc += 2;
            break;
            
        case 0xB000: // Jumps to the address NNN plus V0.
            pc = (opcode & 0x0FFF) + V[0x0];
            break;
            
        case 0xC000: { // Sets VX to a random number, masked by NN.
            V[(opcode & 0x0F00) >> 8] = random() & (opcode & 0x00FF); // I think this is correct
            pc += 2;
            break;
        }
            
        case 0xD000: { // Sprites stored in memory at location in index register (I), maximum 8bits wide. Wraps around the screen. If when drawn, clears a pixel, register VF is set to 1 otherwise it is zero. All drawing is XOR drawing (i.e. it toggles the screen pixels)
            unsigned short x = V[(opcode & 0x0F00) >> 8];
            unsigned short y = V[(opcode & 0x00F0) >> 4];
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;
            unsigned short line;
            
            V[0xF/*15*/] = 0;
            for (int _y = 0; _y < height; _y++) {
                line = memory[I + _y];
                for (int _x = 0; _x < 8; _x++) {
                    pixel = line & (0x80 >> _x);
                    if (pixel != 0) {
                        
                        unsigned short index = ((x + _x) + (y + _y) * 64);
                        
                        if (gfx[index] == 1) {
                            V[0xF/*15*/] = 1;
                        }
                        
                        if (index <= sizeof(gfx)) {
                            gfx[index] ^= 1;
                        }
                        
                    }
                }
            }
            
            draw = true;
            pc += 2;
        }
        break;
            
        case 0xE000:
            
            switch (opcode & 0x000F) {
                case 0x000E: // Skips the next instruction if the key stored in VX is pressed.
                    if (key[V[(opcode & 0x0F00) >> 8]] == 1) {
                        pc += 4;
                    } else {
                        pc += 2;
                    }
                    break;
                    
                case 0x0001: // Skips the next instruction if the key stored in VX isn't pressed.
                    if (key[V[(opcode & 0x0F00) >> 8]] == 0) {
                        pc += 4;
                    } else {
                        pc += 2;
                    }
                    break;
                    
                default:
                    printf("Unknown opcode: 0x%X\n", opcode);
                    break;
            }
            break;
            
        case 0xF000:
            switch (opcode & 0x000F) {
                case 0x0007: // Sets VX to the value of the delay timer.
                    V[(opcode & 0x0F00) >> 8] = delayTimer;
                    pc += 2;
                    break;
                    
                case 0x000A: // A key press is awaited, and then stored in VX.
                    
                    printf("NOT IMPLEMENTED: 0x%X\n", opcode);
                    for (int i = 0; i < sizeof(key); i++) {
                        if (key[i] == 1) {
                            V[(opcode & 0x0F00) >> 8] = key[i];
                            pc += 2;
                        }
                    }
                    break;
                    
                case 0x0005:
                    switch (opcode & 0x00F0) {
                        case 0x0010: // Sets the delay timer to VX.
                            delayTimer = V[(opcode & 0x0F00) >> 8];
                            pc += 2;
                            break;
                            
                        case 0x0050: { // Stores V0 to VX in memory starting at address I.
                             printf("NOT IMPLEMENTED: 0x%X\n", opcode);
//                            break;
                            int _i = 0;
                            for (int i = V[0x0]; i < (V[(opcode & 0x0F00) >> 8] - V[0x0]); i++) {
                                memory[I + _i] = V[i];
                                _i++;
                            }
                            
                            I = I + V[(opcode & 0x0F00) >> 8] + 1;
                            pc += 2;
                            break;
                        }
                            
                        case 0x0060: { // Fills V0 to VX with values from memory starting at address I.
                            for (int i = 0; i < V[(opcode & 0x0F00) >> 8]; i++) {
                                V[i] = memory[I + i];
                            }

                            I = I + V[(opcode & 0x0F00) >> 8] + 1;
                            pc += 2;
                            break;
                        }
                            
                        default:
                            printf("Unknown opcode: 0x%X\n", opcode);
                            break;
                    }
                    break;
                    
                case 0x0008: // Sets the sound timer to VX.
                    soundTimer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                    
                case 0x000E: // Adds VX to I.
                    I += V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                    
                case 0x0009: // Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font.
                    I = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                    
                case 0x0003: // Stores the Binary-coded decimal representation of VX, with the most significant of three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2. (In other words, take the decimal representation of VX, place the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.)
                    memory[I]     = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
                    pc += 2;
                    break;
                    
                default:
                    printf("Unknown opcode: 0x%X\n", opcode);
                    break;
            }
            break;
            
        default:
            printf("Unknown opcode: 0x%X\n", opcode);
            break;
    }
    
//    Update timers
    if (delayTimer > 0) {
        --delayTimer;
    }
    
    if (soundTimer > 0) {
        if (soundTimer == 1) {
            printf("SOUND!!\n");
            --soundTimer;
        }
    }
}

void setKeys() {
    SDL_Event event;
    while( SDL_PollEvent( &event ) ){
        /* We are only worried about SDL_KEYDOWN, SDL_KEYUP, and SDL_QUIT events */
        switch( event.type ){
            case SDL_KEYDOWN:
                switch( event.key.keysym.sym ){
                    case SDLK_LEFT:
                        
                        break;
                    case SDLK_RIGHT:

                        break;
                    case SDLK_UP:
                        
                        break;
                    case SDLK_DOWN:
                        
                        break;
                    default:
                        break;
                }
                break;
                
            case SDL_KEYUP:
                switch( event.key.keysym.sym ){
                    case SDLK_LEFT:
                        
                        break;
                    case SDLK_RIGHT:
                        
                        break;
                    case SDLK_UP:
                        
                        break;
                    case SDLK_DOWN:
                        
                        break;
                    default:
                        break;
                }

                break;
            
            case SDL_QUIT:
                SDL_Quit();
                shutDown = true;
                break;
                
            default:
                break;
        }
    }
}

void drawGraphics() {
    SDL_RenderClear(screenRenderer);
    for (int i = 0; i < sizeof(gfx); i++) {
        if (gfx[i] == 1) {
            SDL_SetRenderDrawColor(screenRenderer, 255, 255, 255, 255);
        } else {
            SDL_SetRenderDrawColor(screenRenderer, 0, 0, 0, 255);
        }
        int x = (i % 64);
        int y = (int)floor(i / 64);
        
        SDL_Rect rect = {x * 10, y * 10, 10, 10};
        SDL_RenderFillRect(screenRenderer, &rect);
    }
    SDL_RenderPresent(screenRenderer);
    draw = false;
}