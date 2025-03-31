import * as fs from 'fs';

/*
  This Atari 2600 ROM is a very simple program that continuously cycles the background color.
  It works by incrementing a seed value stored at zero-page address $80 and then copying that
  value to zero-page address $08, which our emulator maps to the TIA background color (COLUBK).
  
  The program works as follows:

  1. **Increment the Seed and Update the Background Color:**
     - **INC $80 (E6 80):** Increment the seed stored at memory address $80.
       (This causes the seed to wrap from 0xFF back to 0x00.)
     - **LDA $80 (A5 80):** Load the updated seed from $80 into the accumulator (A).
     - **STA $08 (85 08):** Store the value in A into address $08, which is mapped to the
       TIA register controlling the background color. This sets the new background color for the frame.

  2. **Delay Loop (Busy-Wait):**
     - **LDY #$40 (A0 40):** Load the Y register with 0x40 (64). This value is used as the outer
       loop counter for the delay.
     - **LDX #$40 (A2 40):** Load the X register with 0x40 (64). This value is used as the inner
       loop counter for the delay.
     - **DEX (CA):** Decrement the X register by 1.
     - **BNE $000D (D0 FD):** If the result in X is not zero, branch back 3 bytes (0xFD = -3 in two’s complement)
       to the DEX instruction. This inner loop runs 64 times (from 0x40 down to 0).
     - **DEY (88):** After the inner loop completes (when X reaches 0), decrement Y by 1.
     - **BNE $000B (D0 F8):** If Y is not zero, branch back 8 bytes (0xF8 = -8 in two’s complement)
       to the start of the inner loop. This creates a nested loop, with the outer loop running 64 times.
     - The overall delay is 64 * 64 iterations, which is much shorter than the delay produced by 0xFF,
       resulting in faster color changes.

  3. **Restart:**
     - **JMP $F000 (4C 00 F0):** Jump back to address $F000, the start of the ROM code.
       (This assumes that the ROM is loaded at memory location 0xF000.)

  **Overall:**  
  The program continuously:
  - Increments the seed (cycling from 0x00 to 0xFF),
  - Updates the TIA background color register with the new seed value,
  - Waits for a short period using a delay loop,
  - And then jumps back to the start to repeat the process.
  
  This causes the background color to change over time according to the palette mapping defined in the emulator.
*/

const program = [
  0xE6, 0x80,       // INC $80   — Increment the seed at $80.
  0xA5, 0x80,       // LDA $80   — Load the updated seed from $80 into the accumulator.
  0x85, 0x08,       // STA $08   — Store the value to $08 (mapped to the TIA background color register).
  0xA0, 0x40,       // LDY #$40  — Load Y with 0x40 (64 iterations for outer delay loop).
  0xA2, 0x40,       // LDX #$40  — Load X with 0x40 (64 iterations for inner delay loop).
  0xCA,             // DEX       — Decrement X.
  0xD0, 0xFD,       // BNE $000D — Branch back 3 bytes if X ≠ 0 (0xFD = -3).
  0x88,             // DEY       — Decrement Y.
  0xD0, 0xF8,       // BNE $000B — Branch back 8 bytes if Y ≠ 0 (0xF8 = -8).
  0x4C, 0x00, 0xF0  // JMP $F000 — Jump back to the start of the ROM (assuming ROM is loaded at 0xF000).
];

const romSize = 4096; // Standard Atari 2600 ROM size is 4K.

// Create a Buffer from the program array.
let romBuffer = Buffer.from(program);

// Pad the ROM buffer with zeros until it reaches 4096 bytes.
if (romBuffer.length < romSize) {
  const padding = Buffer.alloc(romSize - romBuffer.length, 0);
  romBuffer = Buffer.concat([romBuffer, padding]);
}

// Write the ROM to a file.
fs.writeFileSync('./roms/atari2600/cycle-colors.a26', romBuffer);
console.log(`Wrote ${romBuffer.length} bytes to cycle-colors.a26`);