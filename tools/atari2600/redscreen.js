import * as fs from 'fs';

/*
  This Atari 2600 ROM is a very simple program that repeatedly sets the background 
  color of the screen to 0x80 (mapped to red in the emulator) and then waits for a 
  short period before repeating. The ROM  uses a busy-wait delay loop to hold the 
  color on the screen so you can see it.

  The program does the following steps:

  1. **Initialize the Background Color and Seed:**
     - **LDA #$80:** The program loads the constant value 0x80 into the accumulator (A). 
       This value is chosen because, when processed by our palette lookup, it will map to a 
       specific color (for example, red if our palette is set up that way).
     - **STA $08:** The value in the accumulator (A) is stored into the zero-page address $08. 
       In our emulator, this address is mapped to the TIA register that controls the background 
       color (COLUBK). This immediately sets the screen's background to the new color.
  
  2. **Set Up a Delay Loop:**
     - **LDY #$FF:** The Y register is loaded with 0xFF. This value will serve as the outer loop 
       counter for the delay.
     - **LDX #$FF:** The X register is loaded with 0xFF. This value will serve as the inner loop 
       counter for the delay.
  
  3. **Inner Delay Loop:**
     - **DEX:** The program decrements the X register by 1.
     - **BNE $000D:** If the result in X is not zero, the program branches back 3 bytes 
       (using an offset of 0xFD, which represents -3 in two's complement) to the DEX instruction. 
       This loop (inner loop) rapidly decrements X from 0xFF down to 0.
  
  4. **Outer Delay Loop:**
     - **DEY:** Once the inner loop finishes (X reaches 0), the Y register is decremented by 1.
     - **BNE $000B:** If Y is not zero, the program branches back 8 bytes (using an offset of 
       0xF8, which represents -8) to the beginning of the inner loop. This creates a nested loop 
       structure where the inner loop runs for each iteration of the outer loop, creating a delay.
  
  5. **Restart the Program:**
     - **JMP $0000:** After the delay loops complete (when Y has counted down to 0), the program 
       jumps back to address $0000 to restart the process.
  
  Overall, this program continuously:
  - Sets the background color (using a value loaded into A and stored into TIA register $08),
  - Runs a delay loop so that the color remains on the screen for a perceptible time (approximately one second),
  - And then loops back to repeat the process.
  
  The delay loop is a simple busy-wait loop that does nothing other than consume CPU cycles.
  This is not an efficient way to manage timing in a modern system, but it accurately reflects 
  how many Atari 2600 programs used to generate a visible delay without dedicated timer hardware.
*/

const program = [
  0xA9, 0x80,       // LDA #$80  — Load immediate value 0x80 (red) into the accumulator.
  0x85, 0x08,       // STA $08   — Store the accumulator to zero-page address $08 (this is our TIA register for background color).
  0xA0, 0xFF,       // LDY #$FF  — Load Y register with 0xFF (outer loop counter for delay).
  0xA2, 0xFF,       // LDX #$FF  — Load X register with 0xFF (inner loop counter for delay).
  0xCA,             // DEX       — Decrement X.
  0xD0, 0xFD,       // BNE $000D — Branch to address $000D if result of DEX is nonzero (0xFD = -3 offset).
  0x88,             // DEY       — Decrement Y.
  0xD0, 0xF8,       // BNE $000B — Branch to address $000B if result of DEY is nonzero (0xF8 = -8 offset).
  0x4C, 0x00, 0xF0  // JMP $F000 — Jump to the beginning to repeat the process.
];

const romSize = 4096; // Standard Atari 2600 ROM size is 4K.

// Create a Buffer from the program array.
let romBuffer = Buffer.from(program);

// Pad the ROM buffer with zeros until it reaches 4096 bytes.
if (romBuffer.length < romSize) {
  const padding = Buffer.alloc(romSize - romBuffer.length, 0);
  romBuffer = Buffer.concat([romBuffer, padding]);
}

/*
  The Atari 2600 uses a small amount of internal RAM, and the ROM (cartridge data)
  is typically mapped into a high memory area. In our program, we need a seed value
  to generate pseudo-random background colors. The ROM code first reads a value from
  memory address $80 (a zero-page location), modifies it (using LSR and EOR instructions),
  and writes it back to both $80 (to update the seed) and $08 (our chosen TIA register for
  background color).

  Without explicitly initializing memory at $80, it would likely be 0 (or undefined), so the
  algorithm wouldn’t work correctly. Therefore, we set the value at $80 to 0xAB (a nonzero seed)
  before running the ROM code.
*/
romBuffer[0x80] = 0xAB;

// Write the ROM to a file.
fs.writeFileSync('./roms/atari2600/redscreen.a26', romBuffer);
console.log(`Wrote ${romBuffer.length} bytes to redscreen.a26`);