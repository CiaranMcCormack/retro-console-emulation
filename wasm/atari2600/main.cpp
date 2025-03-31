#include <cstdlib>
#include <ctime>
#include <cstdint>
#include <cstring>
#include <cstdio>

// Define a virtual screen size for output (for demo purposes).
const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 192;

// The screen buffer will hold one 8‑bit value per pixel (grayscale).
uint8_t screen[SCREEN_WIDTH * SCREEN_HEIGHT];

// Atari 2600 cartridges are typically 4K in size.
uint8_t memory[4096];

// For a simple demonstration, we simulate one TIA register: background color (COLUBK).
// In a real Atari 2600, many registers control various parts of the TIA.
uint8_t COLUBK = 0;

// Program counter for the (simplified) CPU.
// A true 6507 emulator would have many more registers and a proper instruction decoder.
uint16_t pc = 0;

extern "C"
{

  /**
   * Initialize the Atari 2600 state.
   *
   * Clears the screen, zeroes memory, and sets the initial program counter.
   */
  void init()
  {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    memset(screen, 0, sizeof(screen));
    memset(memory, 0, sizeof(memory));
    // Start execution at address 0 (for simplicity).
    pc = 0;
    COLUBK = 0;
  }

  /**
   * Load an Atari 2600 ROM into memory.
   *
   * @param romData Pointer to a buffer containing the ROM.
   * @param size    Size of the ROM in bytes (should be 4096 for a standard Atari 2600 ROM).
   */
  void loadProgram(uint8_t *romData, int size)
  {
    if (size > 4096)
    {
      printf("Warning: ROM size (%d bytes) exceeds 4K. Only the first 4096 bytes will be used.\n", size);
      size = 4096;
    }
    memcpy(memory, romData, size);
    // For this simple emulator, reset the program counter.
    pc = 0;
  }

  /**
   * Emulate one cycle of the Atari 2600.
   *
   * In a full emulator, this would decode and execute a 6507 instruction.
   * Here we simulate a test program (such as a color bar generator) by:
   *   - Reading the next byte from memory (from the ROM)
   *   - Using that byte as a background color value.
   *   - Filling the screen buffer with that color.
   */
  void emulateCycle()
  {
    // For demonstration, we read a byte from memory at pc to simulate a TIA register write.
    COLUBK = memory[pc];
    // Advance pc and wrap around the 4K memory.
    pc = (pc + 1) % 4096;

    // Update the screen buffer: fill the entire screen with the current color.
    // Here the value of COLUBK (0-255) is treated as a grayscale value.
    memset(screen, COLUBK, sizeof(screen));
  }

  /**
   * Run a number of cycles based on the elapsed time.
   *
   * @param deltaMs Elapsed time in milliseconds.
   */
  void run(double deltaMs)
  {
    // For simplicity, run a fixed number of cycles per call.
    // A real emulator would adjust cycles based on CPU speed.
    const int cyclesPerCall = 10;
    for (int i = 0; i < cyclesPerCall; i++)
    {
      emulateCycle();
    }
  }

  /**
   * Get a pointer to the current screen buffer.
   *
   * The returned buffer contains one 8‑bit value per pixel (grayscale).
   */
  uint8_t *getScreen()
  {
    return screen;
  }

  /**
   * Get the width of the screen.
   */
  int getScreenWidth()
  {
    return SCREEN_WIDTH;
  }

  /**
   * Get the height of the screen.
   */
  int getScreenHeight()
  {
    return SCREEN_HEIGHT;
  }
} // extern "C"