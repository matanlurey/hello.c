#include <stdbool.h>
#include <stdio.h>

const int TOTAL_MEMORY = 1024 * 4;
const int SCREEN_WIDTH = 64;
const int SCREEN_HEIGHT = 32;

// TODO: Inline a built-in font (see any online guide).

// Represents the running state of the CHIP-8 virtual machine.
typedef struct chip8
{
  // 4-kilobytes of RAM.
  unsigned char memory[TOTAL_MEMORY];

  // A 64x32 pixel monochrome display
  bool display[SCREEN_WIDTH][SCREEN_HEIGHT];

  // Program counter; points to the current instruction in memory.
  unsigned short pc;

  // Index register (points at locations in memory).
  unsigned short index;

  // Used to call subroutines/functions and return from them.
  unsigned short stack[16];

  // Timer; decremented at a rate of 60hz until it reaches 0.
  unsigned char delay;

  // Timer; decremented at a rate of 60hz until it reaches 0; beeps when > 0.
  unsigned char sound;

  // General-purpose registers.
  unsigned char registers[16];
} VM;

void vm_execute(VM *vm)
{
  // Fetch.
  unsigned short opcode = vm->memory[vm->pc] << 8 | vm->memory[vm->pc + 1];

  // Decode.
  unsigned char kind = opcode >> 12;

  switch (kind)
  {
  case 0x0:
    print("Clear screen\n");
    memset(vm->display, 0, sizeof vm->display);
    vm->pc += 2;
    break;
  case 0x1:
    printf("Jump to NNN\n");
    unsigned short offset = opcode & 0x0FFF;
    vm->pc = offset;
    break;
  case 0x6:
    printf("Set register VX to value NN\n");
    unsigned char vx = opcode & 0x0F00 >> 8;
    unsigned short value = opcode & 0x00FF;
    vm->registers[vx] = value;
    vm->pc += 2;
    break;
  case 0x7:
    printf("Increment register VX by value NN\n");
    unsigned char vx = opcode & 0x0F;
    unsigned short value = opcode & 0x00FF;
    vm->registers[vx] += value;
    vm->pc += 2;
    break;
  case 0xA:
    printf("Set index register I to NNN\n");
    unsigned short value = opcode & 0x0FFF;
    vm->index = value;
    break;
  case 0xD:
    printf("Display/draw\n");
    break;
  default:
    printf("Unknown opcode: %X\n", opcode);
    break;
  }
}

int main()
{
  return 0;
}
