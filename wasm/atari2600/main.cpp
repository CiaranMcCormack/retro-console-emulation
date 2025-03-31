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
uint8_t rgbScreen[SCREEN_WIDTH * SCREEN_HEIGHT * 3];

// Atari 2600 cartridges are typically 4K in size.
uint8_t memory[4096];

// For a simple demonstration, we simulate one TIA register: background color (COLUBK).
// In a real Atari 2600, many registers control various parts of the TIA.
// Here we assume that writes to address 0x09 update the background color.
uint8_t COLUBK = 0;

// Program counter (PC) for our simplified CPU.
uint16_t pc = 0;

// ----- CPU Registers (Simplified 6502) -----
// Accumulator
uint8_t A = 0;
// X register
uint8_t X = 0;
// Y register
uint8_t Y = 0;
// Processor status flags:
// Bit 0: Carry, Bit 1: Zero, Bit 2: Interrupt Disable,
// Bit 3: Decimal, Bit 4: Break, Bit 5: Unused, Bit 6: Overflow, Bit 7: Negative.
uint8_t status = 0;
// Stack Pointer (8-bit, typically starts at 0xFF)
uint8_t SP = 0xFF;

// Global variable to control verbosity of opcode logging.
bool verboseLogging = false;

// Define a structure for an RGB color.
struct RGB
{
  uint8_t r, g, b;
};

// Define a palette of 16 colors (a common approximation for Atari 2600 colors).
const int PALETTE_SIZE = 16;
RGB palette[PALETTE_SIZE] = {
    {0x00, 0x00, 0x00}, // 0: Black (#000000) – Maps 0x00 to 0x0F
    {0x1D, 0x2B, 0x53}, // 1: Dark Blue (#1D2B53) – Maps 0x10 to 0x1F
    {0x7E, 0x25, 0x53}, // 2: Dark Purple (#7E2553) – Maps 0x20 to 0x2F
    {0x00, 0x87, 0x51}, // 3: Dark Green (#008751) – Maps 0x30 to 0x3F
    {0xAB, 0x52, 0x36}, // 4: Brown (#AB5236) – Maps 0x40 to 0x4F
    {0x5F, 0x57, 0x4F}, // 5: Dark Gray (#5F574F) – Maps 0x50 to 0x5F
    {0xC2, 0xC3, 0xC7}, // 6: Light Gray (#C2C3C7) – Maps 0x60 to 0x6F
    {0xFF, 0xF1, 0xE8}, // 7: White (#FFF1E8) – Maps 0x70 to 0x7F
    {0xFF, 0x00, 0x00}, // 8: Red (#FF0000) – Maps 0x80 to 0x8F
    {0xFF, 0xA3, 0x00}, // 9: Orange (#FFA300) – Maps 0x90 to 0x9F
    {0xFF, 0xEC, 0x27}, // 10: Yellow (#FFEC27) – Maps 0xA0 to 0xAF
    {0x00, 0xE4, 0x36}, // 11: Bright Green (#00E436) – Maps 0xB0 to 0xBF
    {0x29, 0xAD, 0xFF}, // 12: Sky Blue (#29ADFF) – Maps 0xC0 to 0xCF
    {0x83, 0x76, 0x9C}, // 13: Lavender (#83769C) – Maps 0xD0 to 0xDF
    {0xFF, 0x77, 0xA8}, // 14: Pink (#FF77A8) – Maps 0xE0 to 0xEF
    {0xFF, 0xCC, 0xAA}  // 15: Peach (#FFCCAA) – Maps 0xF0 to 0xFF
};

// ----- Helper Functions for 6502 Flags -----
// Set or clear the Zero flag based on a value.
void updateZeroFlag(uint8_t value)
{
  if (value == 0)
    status |= 0x02; // Set Zero flag (bit 1)
  else
    status &= ~0x02; // Clear Zero flag
}

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
    // Load the ROM into memory starting at 0xF000.
    memcpy(memory + 0xF000, romData, size);
    // Set the program counter to the start of the ROM.
    pc = 0xF000;

    if (verboseLogging)
    {
      printf("Loaded ROM into memory:\n");
      for (int i = 0; i < size; i++)
      {
        printf("%02X ", memory[i]);
        if ((i + 1) % 16 == 0)
          printf("\n");
      }
      if (size % 16 != 0)
        printf("\n");
    }
  }

  /**
   * Emulate one CPU cycle for the Atari 2600.
   *
   * Fetches an opcode from memory at pc, decodes it with a switch statement,
   * and executes it. In our simplified implementation, if an STA instruction writes
   * to address 0x09, we update COLUBK so the screen background will change.
   */
  void emulateCycle()
  {
    // Fetch the opcode (1 byte) from memory at the current program counter.
    uint8_t opcode = memory[pc];
    switch (opcode)
    {
    // 0xA5: LDA Zero Page – Load the accumulator from a zero-page memory location.
    case 0xA5:
    {
      uint8_t zp_addr = memory[pc + 1];
      A = memory[zp_addr];
      updateZeroFlag(A);
      if (verboseLogging)
        printf("LDA $%02X: Load accumulator from zero page address $%02X, A = 0x%02X at pc: 0x%04X\n",
               zp_addr, zp_addr, A, pc);
      pc += 2;
      break;
    }

    // LDA immediate – Load accumulator with immediate value.
    case 0xA9:
    {
      uint8_t operand = memory[pc + 1];
      A = operand;
      updateZeroFlag(A);
      if (verboseLogging)
        printf("LDA #$%02X: A = 0x%02X at pc: 0x%04X\n", operand, A, pc);
      pc += 2;
      break;
    }
    // LSR A – Logical shift right of the accumulator.
    case 0x4A:
    {
      uint8_t carry = A & 0x01;
      A >>= 1;
      if (carry)
        status |= 0x01;
      else
        status &= ~0x01;
      updateZeroFlag(A);
      if (verboseLogging)
        printf("LSR A: A = 0x%02X at pc: 0x%04X\n", A, pc);
      pc += 1;
      break;
    }
    // EOR immediate – Exclusive OR accumulator with immediate value.
    case 0x49:
    {
      uint8_t operand = memory[pc + 1];
      A ^= operand;
      updateZeroFlag(A);
      if (verboseLogging)
        printf("EOR #$%02X: A = 0x%02X at pc: 0x%04X\n", operand, A, pc);
      pc += 2;
      break;
    }
    // STA zero page – Store accumulator into a zero page address.
    case 0x85:
    {
      uint8_t zp_addr = memory[pc + 1];
      memory[zp_addr] = A;
      // If writing to 0x08 or 0x09, update COLUBK.
      if (zp_addr == 0x08 || zp_addr == 0x09)
        COLUBK = A;
      if (verboseLogging)
        printf("STA $%02X: Stored A = 0x%02X at pc: 0x%04X\n", zp_addr, A, pc);
      pc += 2;
      break;
    }
    // LDY immediate – Load Y register with immediate value.
    case 0xA0:
    {
      uint8_t operand = memory[pc + 1];
      Y = operand;
      updateZeroFlag(Y);
      if (verboseLogging)
        printf("LDY #$%02X: Y = 0x%02X at pc: 0x%04X\n", operand, Y, pc);
      pc += 2;
      break;
    }
    // LDX immediate – Load X register with immediate value.
    case 0xA2:
    {
      uint8_t operand = memory[pc + 1];
      X = operand;
      updateZeroFlag(X);
      if (verboseLogging)
        printf("LDX #$%02X: X = 0x%02X at pc: 0x%04X\n", operand, X, pc);
      pc += 2;
      break;
    }
    // DEX – Decrement the X register.
    case 0xCA:
      X -= 1;
      updateZeroFlag(X);
      if (verboseLogging)
        printf("DEX: X = 0x%02X at pc: 0x%04X\n", X, pc);
      pc += 1;
      break;
    // BNE – Branch if Zero flag is clear.
    case 0xD0:
    {
      int8_t offset = static_cast<int8_t>(memory[pc + 1]);
      if ((status & 0x02) == 0)
      {
        if (verboseLogging)
          printf("BNE: Branch taken, offset %d at pc: 0x%04X\n", offset, pc);
        pc = pc + 2 + offset;
      }
      else
      {
        if (verboseLogging)
          printf("BNE: Branch not taken at pc: 0x%04X\n", pc);
        pc += 2;
      }
      break;
    }
    // DEY – Decrement the Y register.
    case 0x88:
      Y -= 1;
      updateZeroFlag(Y);
      if (verboseLogging)
        printf("DEY: Y = 0x%02X at pc: 0x%04X\n", Y, pc);
      pc += 1;
      break;
    // JMP Absolute – Jump to the absolute address specified by the next two bytes.
    case 0x4C:
    {
      uint16_t addr = memory[pc + 1] | (memory[pc + 2] << 8);
      if (verboseLogging)
        printf("JMP $%04X: Jump to absolute address at pc: 0x%04X\n", addr, pc);
      pc = addr;
      break;
    }
      // 0xE6: INC Zero Page – Increment the memory value at a zero-page address.
    case 0xE6:
    {
      uint8_t zp_addr = memory[pc + 1];
      memory[zp_addr]++; // Increment the value at the specified zero-page address.
      updateZeroFlag(memory[zp_addr]);
      // Optionally update the Negative flag based on the result.
      if (memory[zp_addr] & 0x80)
        status |= 0x80;
      else
        status &= ~0x80;
      if (verboseLogging)
        printf("INC $%02X: Incremented memory at zero page address $%02X, new value = 0x%02X at pc: 0x%04X\n",
               zp_addr, zp_addr, memory[zp_addr], pc);
      pc += 2;
      break;
    }
    // Default: Unsupported opcode.
    default:
      printf("Unsupported opcode: 0x%02X at pc: 0x%04X\n", opcode, pc);
      pc += 1;
      break;
    }
  }

  /**
   * Run a number of cycles based on the elapsed time.
   *
   * For simplicity, a fixed number of cycles are run per call.
   * After running cycles, update the screen buffer using the current COLUBK value.
   */
  void run(double deltaMs)
  {
    int cyclesToRun = 1; // For simplicity, run one cycle per call.
    bool normalMode = true;
    if (normalMode)
    {
      // Calculate how many cycles to run given deltaMs.
      const double cyclesPerMs = 1190.0; // Approximately 1,190 cycles per millisecond.
      cyclesToRun = static_cast<int>(deltaMs * cyclesPerMs);
    }
    //  printf("Delta = %f, Running %d cycles\n", deltaMs, cyclesToRun);
    for (int i = 0; i < cyclesToRun; i++)
    {
      emulateCycle();
    }
    // Update the screen buffer with the current background color.
    memset(screen, COLUBK, sizeof(screen));
  }

  /**
   * Get a pointer to the current screen buffer.
   *
   * The returned buffer contains one 8‑bit value per pixel (grayscale).
   */
  uint8_t *getScreen()
  {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    {
      uint8_t pixel = screen[i];
      int index = pixel / 16; // Scale 0–255 to 0–15.
      if (index < 0 || index >= PALETTE_SIZE)
      {
        // Fallback: use magenta.
        rgbScreen[i * 3 + 0] = 255;
        rgbScreen[i * 3 + 1] = 0;
        rgbScreen[i * 3 + 2] = 255;
      }
      else
      {
        rgbScreen[i * 3 + 0] = palette[index].r;
        rgbScreen[i * 3 + 1] = palette[index].g;
        rgbScreen[i * 3 + 2] = palette[index].b;
      }
    }
    return rgbScreen;
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