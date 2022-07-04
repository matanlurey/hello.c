#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>

const int TOTAL_MEMORY = 1024 * 4;
const int SCREEN_WIDTH = 64;
const int SCREEN_HEIGHT = 32;
const int SCREEN_SCALE = 10;
const int GAME_MEMORY_OFFSET = 0x0200;
const int FONT_SIZE = 80;
const unsigned char FONT_SET[FONT_SIZE] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

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

  // Last executed opcode, used for debugging purposes.
  unsigned short last_opcode;

  // Whether we've already printed "=== Infinite Loop ===", i.e. for debugging.
  bool is_infinite_looping;
} VM;

int vm_start(FILE *);
void vm_render(VM *, SDL_Surface *);
void vm_execute(VM *);

int vm_start(FILE *game)
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

  VM *vm = malloc(sizeof(VM));

  // Load game into memory.
  fread(
      vm->memory + GAME_MEMORY_OFFSET,
      1,
      TOTAL_MEMORY - GAME_MEMORY_OFFSET,
      game);
  vm->pc = GAME_MEMORY_OFFSET;

  // Load built-in font into memory.
  for (int i = 0; i < FONT_SIZE; i++)
  {
    vm->memory[i] = FONT_SET[i];
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

      vm_execute(vm);
      vm_render(vm, surface);
      SDL_UpdateWindowSurface(window);
    }
  }

  return 0;
}

void vm_debug_display(VM *vm)
{
  for (int y = 0; y < SCREEN_HEIGHT; y++)
  {
    for (int x = 0; x < SCREEN_WIDTH; x++)
    {
      if (vm->display[y][x])
      {
        printf("*");
      }
      else
      {
        printf(" ");
      }
    }
    printf("\n");
  }
}

void vm_render(VM *vm, SDL_Surface *surface)
{
  SDL_LockSurface(surface);

  // Clear the screen.
  Uint32 *screen = (Uint32 *)surface->pixels;
  memset(screen, 0, surface->w * surface->h * sizeof(Uint32));

  // Draw set pixels.
  for (int y = 0; y < SCREEN_HEIGHT * SCREEN_SCALE; y++)
  {
    for (int x = 0; x < SCREEN_WIDTH * SCREEN_SCALE; x++)
    {
      bool pixel = vm->display[y / SCREEN_SCALE][x / SCREEN_SCALE];
      screen[y + x * surface->w] = pixel ? 0xFFFFFFFF : 0;
    }
  }

  SDL_UnlockSurface(surface);
  SDL_Delay(15);
}

// Given the instruction 0xD{XYN}, reads:
//
// `X`, where X is the register holding the X-coordinate.
// `Y`, where Y is the register holding the Y-coordinate.
// `N`, where N is the height of the sprite.
//
// ... ands draws the cooresponding sprite to the (virtual) display.
char *vm_execute_draw(VM *vm, short opcode)
{
  char height = opcode & 0x000F;
  char vx = vm->registers[opcode & 0x0F00 >> 8] % SCREEN_WIDTH;
  char vy = vm->registers[opcode & 0x00F0 >> 4] % SCREEN_HEIGHT;

  // Will be set to 1 IFF any pixels were turned "off" by drawing.
  char vf = 0;

  for (int y = 0; y < height; y++)
  {
    char sprite = vm->memory[vm->index + y];
    for (int x = 0; x < 8; x++)
    {
      bool pixel = sprite & (0x80 >> x);
      if (pixel)
      {
        bool was_set = vm->display[vy + y][vx + x];
        if (was_set)
        {
          vf = 1;
        }

        if (vy + y < SCREEN_HEIGHT && vx + x < SCREEN_WIDTH)
        {
          vm->display[vy + y][vx + x] ^= 1;
        }
      }
    }
  }

  vm->registers[0xF] = vf;

  char *output;
  asprintf(&output, "{X = %d, Y = %d, H = %d}", vx, vy, height);
  vm_debug_display(vm);
  return output;
}

void vm_execute(VM *vm)
{
  // Fetch.
  unsigned short opcode = vm->memory[vm->pc] << 8 | vm->memory[vm->pc + 1];

  // Decode.
  unsigned char kind = opcode >> 12;
  unsigned short last_pc = vm->pc;
  char *text = "UNKNOWN";
  char *debug = "";

  // Execute.
  switch (kind)
  {
  case 0x0:
    text = "CLR_SCRN";
    memset(vm->display, 0, sizeof vm->display);
    vm->pc += 2;
    break;
  case 0x1:
    text = "JUMP";
    vm->pc = opcode & 0x0FFF;
    asprintf(&debug, "-> 0x%04X", vm->pc);
    break;
  case 0x6:
    text = "SET_REG";
    vm->registers[opcode & 0x0F00 >> 8] = opcode & 0x00FF;
    vm->pc += 2;
    asprintf(&debug, "r[%X] = 0x%04X", opcode & 0x0F00 >> 8, opcode & 0x00FF);
    break;
  case 0x7:
    text = "ADD_REG";
    vm->registers[opcode & 0x0F] += opcode & 0x00FF;
    vm->pc += 2;
    asprintf(&debug, "r[%X] += 0x%04X", opcode & 0x0F00 >> 8, opcode & 0x00FF);
    break;
  case 0xA:
    text = "SET_INDEX";
    vm->index = opcode & 0x0FFF;
    vm->pc += 2;
    asprintf(&debug, "i = 0x%04X", opcode & 0x0FFF);
    break;
  case 0xD:
    text = "DRAW";
    debug = vm_execute_draw(vm, opcode);
    vm->pc += 2;
    break;
  default:
    break;
  }

  if (opcode == vm->last_opcode)
  {
    if (!vm->is_infinite_looping)
    {
      printf("========== Looping ==========\n");
      vm->is_infinite_looping = true;
    }
  }
  else
  {
    vm->is_infinite_looping = false;
    printf("%-10s @0x%04X: 0x%04X\n", text, last_pc, opcode);
    if (strcmp(debug, ""))
    {
      printf("%-10s %s\n", "", debug);
    }
  }

  vm->last_opcode = opcode;
}

int main()
{
  char name[100] = "games/IBM_Logo.ch8";
  /*
  printf("Enter the name of the game: ");
  scanf("%s", name);
  */

  FILE *game = fopen(name, "rb");
  if (!game)
  {
    printf("Could not load %s.\n", name);
    fflush(stdin);
    exit(1);
  }

  int result = vm_start(game);
  if (result < 0)
  {
    return result;
  }
  return 0;
}
