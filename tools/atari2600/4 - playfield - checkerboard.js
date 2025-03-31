import * as fs from 'fs';

/*
  Checkerboard Playfield ROM

  This ROM is designed to test playfield rendering by creating a checkerboard (alternating) pattern.
  It writes a fixed value into the TIA playfield registers:
    - PF0 at $0D, PF1 at $0E, PF2 at $0F are all set to 0xAA (binary 10101010).
  When these values are combined and mirrored, they form an alternating pattern.
  
  Additionally:
  - The background color is set via $08 (using LDA #$10, STA $08), which may map to dark blue.
  - A delay loop (using LDY and LDX loaded with 0x10) creates a short busy-wait delay.
  - The program then jumps back to $F000 to repeat the process.
  
  This ROM will help verify that your emulator correctly interprets writes to the playfield registers 
  and renders a symmetric, alternating (checkerboard) playfield pattern.
*/

const program = [
  // Set playfield registers to a checkerboard pattern.
  0xA9, 0xAA,       // LDA #$AA  — Load 0xAA into A.
  0x85, 0x0D,       // STA $0D   — Store A into PF0.
  0xA9, 0xAA,       // LDA #$AA  — Load 0xAA into A.
  0x85, 0x0E,       // STA $0E   — Store A into PF1.
  0xA9, 0xAA,       // LDA #$AA  — Load 0xAA into A.
  0x85, 0x0F,       // STA $0F   — Store A into PF2.

  // Set background color.
  0xA9, 0x10,       // LDA #$10  — Load 0x10 into A (background color index).
  0x85, 0x08,       // STA $08   — Store A into $08 (TIA background color).

  // Delay Loop
  0xA0, 0x10,       // LDY #$10  — Outer loop counter.
  0xA2, 0x10,       // LDX #$10  — Inner loop counter.
  0xCA,             // DEX       — Decrement X.
  0xD0, 0xFD,       // BNE $000D — Branch back 3 bytes if X is not zero.
  0x88,             // DEY       — Decrement Y.
  0xD0, 0xF8,       // BNE $000B — Branch back 8 bytes if Y is not zero.

  // Restart the program.
  0x4C, 0x00, 0xF0  // JMP $F000 — Jump back to the start (ROM loaded at $F000).
];

const romSize = 4096;
let romBuffer = Buffer.from(program);
if (romBuffer.length < romSize) {
  const padding = Buffer.alloc(romSize - romBuffer.length, 0);
  romBuffer = Buffer.concat([romBuffer, padding]);
}

// Initialize seed at $80 for consistency (even though this ROM doesn't use it for randomness).
romBuffer[0x80] = 0xAB;

fs.writeFileSync('./roms/atari2600/4 - playfield - checkerboard.a26', romBuffer);
console.log(`Wrote ${romBuffer.length} bytes to 4 - playfield - checkerboard.a26`);