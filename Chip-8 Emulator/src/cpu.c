#include "cpu.h"


#define MEM_SIZE 4096

#define vF v[0xF]
/*----- Macros -----*/
#define cpu_fetch() (memory[pc] << 8 | memory[pc+1])

#define cpu_getXReg(op) (((op) & 0x0F00) >> 8)
#define cpu_getYReg(op) (((op) & 0x00F0) >> 4)

#define cpu_getN(op) ((op) & 0x000F)
#define cpu_getNN(op) ((op) & 0x00FF)
#define cpu_getNNN(op) ((op) & 0x0FFF)

// Store current opcode
unsigned short opcode;

// Memory: 4k in total
unsigned char memory[MEM_SIZE];

// CPU registers
unsigned char v[16];

// Index register 0x000 to 0xFFF
unsigned short I;
// Program counter
unsigned short pc;

// Array for graphics 64 x 32
unsigned char gfx[64 * 32];
unsigned char drawFlag;

// Timer registers (count at 60 Hz)
unsigned char delay_timer;
unsigned char sound_timer;

// Stack and stack pointer
unsigned short stack[16];
unsigned short sp;

// HEX based keypad (0x0-0xF)
unsigned char key[16];
// Check out for errors
void cpu_initialize(){
	pc  	= 0x200; // Program counter starts at 0x200
	opcode  = 0;	 // Reset current opcode
	I 		= 0;	 // Reset index register
	sp 		= 0;	 // Reset stackpointer

	// Clear display
	for(int i = 0; i < 2048; i++){
		gfx[i] = 0;
	}
	// Clear Stack
	for(int i = 0; i < 16; i++){
		stack[i] = 0;
	}

	// Clear memory
	for(int i = 0; i < MEM_SIZE; i++){
		memory[i] = 0;
	}

	// Clear registers
	for(int i = 0; i < 16; i++){
		v[i] = 0;
	}

	// Load fontset

	// Reset timers
	srand(time(NULL));
	delay_timer = 0;
	sound_timer = 0;

	//Clear screen once
	drawFlag = true;
}

void cpu_loadGame(){
	// Example
	unsigned char file[5];
	unsigned filesize = 10;

	for(int i = 0; i < filesize; i++)
		memory[i+512] = file[i];

}

void emulateCycle(){

	// Fetch opcode
	opcode = cpu_fetch(); // memory[pc] << 8 | memory[pc+1]

	// Decode opcode
	switch (opcode & 0xF000){
		// Some opcodes
		case 0x0000:
			switch (opcode & 0x000F){
			case 0x0000: // 0x00E0: Clears the screen
				// Execute code
			break;

			case 0x000E: // 0x00EE: Returns from subroutine
				pc = stack[sp];
				--sp;
				pc += 2;
			break;

			default:
				printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
			}
		break;

		case 0x1000: // 1NNN [Flow]: jumps to address NNN
			pc = opcode | 0x0FFF;
		break;

		case 0x2000:
			stack[sp] = pc;
			++sp;
			pc = opcode & 0x0FFF;
		break;

		case 0x3000: // 3XNN [Cond]: skips the next instruction if VX equals NN
			if(v[cpu_getXReg(opcode)] == v[cpu_getNN(opcode)])
				pc +=4;
			else
				pc +=2;
		break;

		case 0x4000: // 4XNN [Cond]: skips the next instruction if VX doesn't equal NN
			if(v[cpu_getXReg(opcode)] != cpu_getNN(opcode))
				pc += 4;
			else
				pc +=2;
		break;

		case 0x5000: // 5XY0 [Cond]: skips the next instruction if VX equals VY
			if(v[cpu_getXReg(opcode)] == v[cpu_getYReg(opcode)])
				pc += 4;
			else
				pc += 2;
		break;

		case 0x6000: // 6XNN [Const]: sets VX to NN
			v[cpu_getXReg(opcode)] = cpu_getNN(opcode);
			pc += 2;
		break;

		case 0x7000: // 7XNN [Const]: adds NN to VX
			v[cpu_getXReg(opcode)] += cpu_getNN(opcode);
			pc += 2;
		break;

		case 0x8000:
			switch (opcode & 0x000F){
				case 0x000: // 8XY0 [Assign]: sets VX to the value of VY
					v[cpu_getXReg(opcode)] = v[cpu_getYReg(opcode)];
					pc += 2;
				break;

				case 0x0001: // 8XY1 [BitOp]: sets VX to VX or VY. (Bitwise OR operation)
					v[cpu_getXReg(opcode)] |= v[cpu_getYReg(opcode)];
					pc += 2;
				break;

				case 0x0002: // 8XY2 [BitOp]: sets VX to VX and VY. (Bitwise AND operation)
					v[cpu_getXReg(opcode)] &= v[cpu_getYReg(opcode)];
					pc += 2;
				break;

				case 0x0003: // 8XY3 [BitOp]: sets VX to VX xor VY. (Bitwise XOR operation)
					v[cpu_getXReg(opcode)] ^= v[cpu_getXReg(opcode)];
					pc += 2;
				break;

				case 0x0004: // 8XY4 [Math]: adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't
					if(v[cpu_getYReg(opcode)] > (0xFF - v[cpu_getXReg(opcode)]))
						v[0xF] = 1; // Carry
					else
						v[0xF] = 0;
					v[cpu_getXReg(opcode)] += v[cpu_getYReg(opcode)]; //VX += Vy
				break;

				case 0x0005: // 8XY5 [Math]: VY is subtracted from VX. VF is set to 0 when there's a borrow, and to 1 when there isn't
					if(v[cpu_getYReg(opcode)] > v[cpu_getXReg(opcode)])
						vF = 0;
					else
						vF = 1;
					v[cpu_getXReg(opcode)] -= v[cpu_getYReg(opcode)];
					pc += 2;
				break;

				case 0x0006: // 8XY6 [Math]: shifts VX right by one. VF is set to the value of the LSB of VX before the shift
					if((v[cpu_getXReg(opcode)] % 2) == 1)
						vF = 1;
					else
						vF = 0;
					v[cpu_getXReg(opcode)] = v[cpu_getXReg(opcode)] >> 1;
					pc += 2;
				break;

				case 0x0007: // 8XY7 [Math]: sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't
					if(v[cpu_getXReg(opcode)] > v[cpu_getYReg(opcode)])
						vF = 0;
					else
						vF = 1;
					v[cpu_getXReg(opcode)] -= v[cpu_getYReg(opcode)];
					pc += 2;
				break;

				case 0x000E: // 8XYE [BitOp]: shifts VX left by one. VF is set to the value of the MSB of VX before the shift
					v[cpu_getXReg(opcode)] = v[cpu_getXReg(opcode)] << 1;
					pc += 2;
				break;
			}
		break;

		case 0x9000: // 9XY0 [Cond]: skips the next instruction if VX doesn't equal VY
			if(v[cpu_getXReg(opcode)] != v[cpu_getYReg(opcode)])
				pc += 4;
			else
				pc += 2;
		break;

		case 0xA000: // ANNN [Mem]: sets I to the address NNN
			I = cpu_getNNN(opcode);
			pc += 2;
		break;

		case 0xB000: // BNNN [Flow]: jumps to the address NNN plus V0
			pc = v[0x0] + cpu_getNNN(opcode);
		break;

		case 0xC000: // CXNN [Rand]: sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
			v[cpu_getXReg(opcode)] = (rand() % 0xFF) & cpu_getNN(opcode);
			pc += 2;
		break;

		case 0xD000: // DXYN [Disp]: draws a sprite at coordinate (VX, VY)
		{
			unsigned short x = v[cpu_getXReg(opcode)];
			unsigned short y = v[cpu_getYReg(opcode)];
			unsigned short height = v[cpu_getN(opcode)];
			unsigned short pixel;

			vF = 0;

			for(int yline = 0; yline < height; yline++){
				pixel = memory[I + yline];
				for(int xline = 0; xline < 8; xline++){
					if((pixel & (0x80 >> xline)) != 0){
						if(gfx[(x + xline + ((y + yline) * 64))] == 1)
							vF = 1;
						gfx[x + xline + ((y + yline) * 64)] ^= 1;
					}
				}
			}

			drawFlag = true;
			pc += 2;
		break;
		}

		case 0xE000:
			switch(opcode & 0x000F){
				case 0x000E: // EX9E [KeyOp]: skips the next instruction if the key stored in VX is pressed
					// MISSING!
				break;

				case 0x001: // EXA1 [KeyOp]: skips the next instruction if the key stored in VX isn't pressed
					// MISSING!
				break;

				default:
					printf("Unknown opcode: 0x%X\n", opcode);
			}
		break;

		case 0xF000:
			switch(opcode & 0x000F){
				case 0x0007: // FX07 [Timer]: sets VX to the value of the delay timer
					v[cpu_getXReg(opcode)] = delay_timer;
					pc += 2;
				break;

				case 0x000A: // FX0A [KeyOp]: a key press is awaited, and then stored in VX
					// MISSING!
				break;

				case 0x0008: // FX18 [Sound]: sets the sound time to VX
					sound_timer = v[cpu_getXReg(opcode)];
					pc += 2;
				break;

				case 0x000E: // FX1E [Mem]: adds VX to I
					I += v[cpu_getXReg(opcode)];
					pc += 2;
				break;

				case 0x0009: // FX29 [Mem]: sets I to the location of the sprite for the character in VX
					// MISSING!
				break;

				case 0x0003: // FX33 [BCD]: stores the BCD representation of VX ... (Check Wikipedia :D)
					memory[I]		= v[cpu_getXReg(opcode)] / 100;
					memory[I + 1]	= (v[cpu_getXReg(opcode)] / 10) % 10;
					memory[I + 2]	= (v[cpu_getXReg(opcode)] % 100) % 10;
					pc += 2;
				break;

				case 0x0005:
					switch(opcode & 0x00F0){
						case 0x0010: // FX07 [Timer]: sets VX to the value of the delay timer
							delay_timer = v[cpu_getXReg(opcode)];
							pc += 2;
						break;

						case 0x0050: // FX55 [Mem]: stores V0 to VX (including VX) in memory starting at address I
							for(int i = 0; i < cpu_getXReg(opcode); i++){
								memory[I + i] = v[i];
							}
							I += cpu_getXReg(opcode) +1;
							pc += 2;
						break;

						case 0x0060: // FX65 [Mem]: fills V0 to VX (including VX) with values from memory starting at address I
							for(int i = 0; i < cpu_getXReg(opcode); i++){
								v[i] = memory[I + i];
							}
							I += cpu_getXReg(opcode) + 1;
							pc += 2;
						break;

						default:
							printf("Unknown opcode :0x%X\n", opcode);
					}
				break;
			}
		break;

		default:
			printf("Unknown opcode: 0x%X\n", opcode);

	}
	/*----- End of Switch -----*/

	if (delay_timer > 0){
		--delay_timer;
	}

	if (sound_timer > 0){
		if(sound_timer == 1){
			printf("Beep!\n");
		}
		--sound_timer;
	}
}