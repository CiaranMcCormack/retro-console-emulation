import * as fs from 'fs';

/*
  Vertical Stripes Playfield ROM

  This ROM demonstrates a vertical stripes playfield pattern. It does the following:

  1. **Set Playfield Pattern:**
     - LDA #$55, STA $0D: Sets PF0 (at address $0D) to 0x55.
     - LDA #$AA, STA $0E: Sets PF1 (at address $0E) to 0xAA.
     - LDA #$FF, STA $0F: Sets PF2 (at address $0F) to 0xFF.
     When combined and mirrored, these values form a symmetric vertical stripe pattern.
  
  2. **Set Background Color:**
     - LDA #$80, STA $08: Sets the background color to 0x80 (which maps to red via the palette).
  
  3. **Delay Loop:**
     A simple busy‑wait loop is executed using LDY and LDX loaded with 0x10, creating a short delay.
  
  4. **Restart:**
     - JMP $F000: Jumps back to the start of the ROM (assuming the ROM is mapped at address $F000).
  
  This ROM allows you to verify that your emulator correctly reads the playfield registers and overlays a vertical stripes pattern on top of the background.
*/

const program = [
  0xA9, 0x55,       // LDA #$55  — Load 0x55 into A.
  0x85, 0x0D,       // STA $0D   — Store A into PF0.
  0xA9, 0xAA,       // LDA #$AA  — Load 0xAA into A.
  0x85, 0x0E,       // STA $0E   — Store A into PF1.
  0xA9, 0xFF,       // LDA #$FF  — Load 0xFF into A.
  0x85, 0x0F,       // STA $0F   — Store A into PF2.
  0xA9, 0x80,       // LDA #$80  — Load 0x80 into A (background color).
  0x85, 0x08,       // STA $08   — Store A into $08 (TIA background color).
  0xA0, 0x10,       // LDY #$10  — Outer loop counter for delay.
  0xA2, 0x10,       // LDX #$10  — Inner loop counter for delay.
  0xCA,             // DEX       — Decrement X.
  0xD0, 0xFD,       // BNE $000D — Branch back 3 bytes if X ≠ 0 (0xFD = -3).
  0x88,             // DEY       — Decrement Y.
  0xD0, 0xF8,       // BNE $000B — Branch back 8 bytes if Y ≠ 0 (0xF8 = -8).
  0x4C, 0x00, 0xF0  // JMP $F000 — Jump back to the start of the ROM.
];

const romSize = 4096;
let romBuffer = Buffer.from(program);
if (romBuffer.length < romSize) {
  const padding = Buffer.alloc(romSize - romBuffer.length, 0);
  romBuffer = Buffer.concat([romBuffer, padding]);
}

// Although this ROM uses fixed playfield patterns, we initialize address $80 for consistency.
// (In this ROM, the seed is not used for random behavior.)
romBuffer[0x80] = 0xAB;

fs.writeFileSync('./roms/atari2600/3 - playfield - vertical stripes.a26', romBuffer);
console.log(`Wrote ${romBuffer.length} bytes to 3 - playfield - vertical stripes.a26`);