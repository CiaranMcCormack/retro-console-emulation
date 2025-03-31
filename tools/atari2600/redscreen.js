import * as fs from 'fs';

// Assemble the 6502 machine code for the ROM.
// The code is as follows (with addresses for clarity):
//
// $0000: A5 80       ; LDA $80         ; Load seed from $80.
// $0002: 4A          ; LSR A           ; Shift right (carry <- bit0).
// $0003: 49 1D       ; EOR #$1D        ; XOR with 0x1D to mix bits.
// $0005: 85 80       ; STA $80         ; Store new seed back to $80.
// $0007: 85 08       ; STA $08         ; Write seed to $08 (background color).
// $0009: A0 FF       ; LDY #$FF        ; Outer loop counter = $FF.
// $000B: A2 FF       ; LDX #$FF        ; Inner loop counter = $FF.
// $000D: CA          ; DEX             ; Decrement X.
// $000E: D0 FD       ; BNE $000D       ; If X != 0, branch back one byte.
// $000F: 88          ; DEY             ; Decrement Y.
// $0010: D0 F8       ; BNE $000B       ; If Y != 0, branch back 8 bytes.
// $0012: 4C 00 00    ; JMP $0000       ; Jump to start.
//
// Also, we initialize the seed at memory location $80 to a nonzero value (0xAB).
const program = [
  0xA5, 0x80,       // LDA $80
  0x4A,             // LSR A
  0x49, 0x1D,       // EOR #$1D
  0x85, 0x80,       // STA $80
  0x85, 0x08,       // STA $08   ; set background color (TIA register)
  0xA0, 0xFF,       // LDY #$FF
  0xA2, 0xFF,       // LDX #$FF
  0xCA,             // DEX
  0xD0, 0xFD,       // BNE $000D  (branch back 3 bytes; 0xFD = -3)
  0x88,             // DEY
  0xD0, 0xF8,       // BNE $000B  (branch back 8 bytes; 0xF8 = -8)
  0x4C, 0x00, 0x00  // JMP $0000
];

const romSize = 4096; // Atari 2600 ROM size

// Create a Buffer from the program array.
let romBuffer = Buffer.from(program);

// Pad the buffer with zeros until it is 4096 bytes long.
if (romBuffer.length < romSize) {
  const padding = Buffer.alloc(romSize - romBuffer.length, 0);
  romBuffer = Buffer.concat([romBuffer, padding]);
}

// Initialize the seed at zero-page $80 with a nonzero value (e.g., 0xAB).
romBuffer[0x80] = 0xAB;

// Write the buffer to a file.
fs.writeFileSync('./roms/atari2600/redscreen.a26', romBuffer);
console.log(`Wrote ${romBuffer.length} bytes to redscreen.a26`);