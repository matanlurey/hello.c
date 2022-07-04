#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>

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

int vm_start()
{
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    printf("Failed to initialize SDL2: %s\n", SDL_GetError());
    return -1;
  }

  SDL_Window *window = SDL_CreateWindow(
      "CHIP-8",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      SCREEN_WIDTH * 10,
      SCREEN_HEIGHT * 10,
      0);

  if (!window)
  {
    printf("Failed to create window: %s\n", SDL_GetError());
    return -1;
  }

  SDL_Surface *surface = SDL_GetWindowSurface(window);

  if (!surface)
  {
    printf("Failed to get surface from window: %s\n", SDL_GetError());
    return -1;
  }

  bool quit = false;
  while (!quit)
  {
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0)
    {
      switch (e.type)
      {
      case SDL_QUIT:
        quit = true;
        break;
      }

      SDL_UpdateWindowSurface(window);
    }
  }

  return 0;
}

void vm_execute(VM *vm)
{
  // Fetch.
  unsigned short opcode = vm->memory[vm->pc] << 8 | vm->memory[vm->pc + 1];

  // Decode.
  unsigned char kind = opcode >> 12;

  switch (kind)
  {
  case 0x0:
    printf("Clear screen\n");
    memset(vm->display, 0, sizeof vm->display);
    vm->pc += 2;
    break;
  case 0x1:
    printf("Jump to NNN\n");
    vm->pc = opcode & 0x0FFF;
    break;
  case 0x6:
    printf("Set register VX to value NN\n");
    vm->registers[opcode & 0x0F00 >> 8] = opcode & 0x00FF;
    vm->pc += 2;
    break;
  case 0x7:
    printf("Increment register VX by value NN\n");
    vm->registers[opcode & 0x0F] += opcode & 0x00FF;
    vm->pc += 2;
    break;
  case 0xA:
    printf("Set index register I to NNN\n");
    vm->index = opcode & 0x0FFF;
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
  int result = vm_start();
  if (result < 0)
  {
    return result;
  }
  printf("Press any key to continue...\n");
  getchar();
  return 0;
}
