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
    memcpy(memory, romData, size);

    // Reset the program counter.
    pc = 0;

    if (!verboseLogging)
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
    // LDA Zero Page – Load accumulator from zero-page.
    case 0xA5:
    {
      uint8_t zp_addr = memory[pc + 1];
      A = memory[zp_addr];
      updateZeroFlag(A);
      if (verboseLogging)
        printf("LDA $%02X: A = 0x%02X at pc: 0x%04X\n", zp_addr, A, pc);
      pc += 2;
      break;
    }
    // LSR A – Logical shift right the accumulator.
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
    // EOR Immediate – Exclusive OR accumulator with immediate value.
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
    // STA Zero Page – Store accumulator into zero page.
    case 0x85:
    {
      uint8_t zp_addr = memory[pc + 1];
      memory[zp_addr] = A;
      // Update background color if writing to address 0x08 (or 0x09, if desired).
      if (zp_addr == 0x08 || zp_addr == 0x09)
      {
        printf("Setting COLUBK to 0x%02X\n", A);
        COLUBK = A;
      }
      if (verboseLogging)
        printf("STA $%02X: Stored A = 0x%02X at pc: 0x%04X\n", zp_addr, A, pc);
      pc += 2;
      break;
    }
    // LDY Immediate – Load Y register with an immediate value.
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
    // LDX Immediate – Load X register with an immediate value.
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
    // BNE – Branch if the Zero flag is clear.
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
    // JMP Absolute – Jump to a specified absolute address.
    case 0x4C:
    {
      uint16_t addr = memory[pc + 1] | (memory[pc + 2] << 8);
      if (verboseLogging)
        printf("JMP $%04X: Jumping to address $%04X at pc: 0x%04X\n", addr, addr, pc);
      pc = addr;
      break;
    }
    // Default: For any unsupported opcode, print an error and advance pc by 1.
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
    // Calculate how many cycles to run given deltaMs.
    // const double cyclesPerMs = 1190.0; // Approximately 1,190 cycles per millisecond.
    // int cyclesToRun = static_cast<int>(deltaMs * cyclesPerMs);
    int cyclesToRun = 1; // For simplicity, run one cycle per call.
    // printf("Delta = %f, Running %d cycles\n", deltaMs, cyclesToRun);
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