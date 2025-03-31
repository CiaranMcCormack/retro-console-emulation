import Stats from 'stats.js';
import { createProgram, setupBuffers } from '../utils/graphics';

/* ============================================================
   Constants and Global Variables
============================================================ */
const chip8KeyMap: Record<string, number> = {
  Digit1: 0x1, Digit2: 0x2, Digit3: 0x3, Digit4: 0xC,
  KeyQ: 0x4, KeyW: 0x5, KeyE: 0x6, KeyR: 0xD,
  KeyA: 0x7, KeyS: 0x8, KeyD: 0x9, KeyF: 0xE,
  KeyZ: 0xA, KeyX: 0x0, KeyC: 0xB, KeyV: 0xF
};

let rom: Uint8Array | null = null;
let soundEnabled = true;
let oscillator: OscillatorNode | null = null;
const audioCtx = new AudioContext();

/* ============================================================
   Utility Functions
============================================================ */

/**
 * Updates sound output based on the Chip8 sound timer.
 */
function updateSound(Module: any) {
  const st = Module._getSoundTimer();
  if (soundEnabled && st > 0) {
    if (!oscillator) {
      const osc = audioCtx.createOscillator();
      osc.type = 'square';
      osc.frequency.value = 440;
      osc.connect(audioCtx.destination);
      osc.start();
      oscillator = osc;
    }
  } else if (oscillator) {
    oscillator.stop();
    oscillator.disconnect();
    oscillator = null;
  }
}

/**
 * Creates and inserts the sound checkbox (with label) into the DOM.
 * This is called only after a ROM is loaded.
 */
function createSoundCheckbox(): HTMLInputElement {
  const soundCheckbox = document.createElement('input');
  soundCheckbox.type = 'checkbox';
  soundCheckbox.checked = soundEnabled;
  soundCheckbox.id = 'soundToggle';

  const label = document.createElement('label');
  label.htmlFor = 'soundToggle';
  label.textContent = "Sound";

  // Insert the checkbox and label before the canvas element.
  const canvas = document.getElementById('glCanvas');
  if (canvas && canvas.parentElement) {
    canvas.parentElement.insertBefore(label, canvas);
    canvas.parentElement.insertBefore(soundCheckbox, canvas);
  }
  
  soundCheckbox.addEventListener('change', () => {
    soundEnabled = soundCheckbox.checked;
  });

  return soundCheckbox;
}

/**
 * Initializes the emulator: loads the ROM, sets up WebGL,
 * creates shaders, and starts the main emulation loop.
 */
function initEmulator(rom: Uint8Array, Module: any, canvas: HTMLCanvasElement) {
  // Load the ROM into emulator memory.
  const ptr = Module._malloc(rom.length);
  Module.HEAPU8.set(rom, ptr);
  Module._loadProgram(ptr, rom.length);
  Module._free(ptr);

  // Set up WebGL context and adjust canvas size.
  const gl = canvas.getContext('webgl');
  if (!gl) {
    console.error("WebGL not supported in this browser.");
    return;
  }
  const width = Module._getScreenWidth();
  const height = Module._getScreenHeight();
  canvas.width = width * 10;
  canvas.height = height * 10;
  gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);

  // Create shaders and buffers.
  const program = createProgram(gl);
  gl.useProgram(program);
  setupBuffers(gl, program);

  // Set up performance stats in the top right.
  const stats = new Stats();
  stats.showPanel(0);
  stats.dom.style.position = 'fixed';
  stats.dom.style.top = '0px';
  stats.dom.style.left = 'auto';
  stats.dom.style.right = '0px';
  document.body.appendChild(stats.dom);

  // Set up keyboard listeners.
  const handleKey = (fn: string) => (e: KeyboardEvent) => {
    const key = chip8KeyMap[e.code];
    if (key !== undefined) {
      Module[`_${fn}`](key);
    }
  };
  document.addEventListener('keydown', handleKey('setKeyDown'));
  document.addEventListener('keyup', handleKey('setKeyUp'));

  // Emulation loop.
  let last = performance.now();
  function loop() {
    if (!gl) return;

    
    stats.begin();
    const now = performance.now();
    const delta = now - last;
    last = now;

    Module._run(delta);

    // Render emulator screen.
    const screenPtr = Module._getScreen();
    const pixels = new Uint8Array(Module.HEAPU8.buffer, screenPtr, width * height);
    const img = new Uint8Array(width * height * 4);
    for (let i = 0; i < pixels.length; i++) {
      const v = pixels[i] ? 255 : 0;
      img[i * 4] = v;
      img[i * 4 + 1] = v;
      img[i * 4 + 2] = v;
      img[i * 4 + 3] = 255;
    }
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, img);
    gl.drawArrays(gl.TRIANGLES, 0, 6);

    updateSound(Module);
    stats.end();
    requestAnimationFrame(loop);
  }
  requestAnimationFrame(loop);

  // Cleanup on page unload.
  window.addEventListener('unload', () => {
    document.removeEventListener('keydown', handleKey('setKeyDown'));
    document.removeEventListener('keyup', handleKey('setKeyUp'));
    document.body.removeChild(stats.dom);
  });
}

/**
 * Sets up a file input to allow the user to load a ROM.
 */
function setupFileLoader(Module: any, canvas: HTMLCanvasElement) {
  const fileInput = document.createElement('input');
  fileInput.type = 'file';
  fileInput.accept = ''; // Allow any file type.
  fileInput.style.cssText = `
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    z-index: 1000;
  `;
  document.body.appendChild(fileInput);
  canvas.style.display = 'none';

  fileInput.addEventListener('change', (e: Event) => {
    const target = e.target as HTMLInputElement;
    if (target.files && target.files.length > 0) {
      const file = target.files[0];
      const reader = new FileReader();
      reader.onload = (event) => {
        const arrayBuffer = event.target?.result;
        if (arrayBuffer && arrayBuffer instanceof ArrayBuffer) {
          rom = new Uint8Array(arrayBuffer);
          fileInput.remove();
          canvas.style.display = 'block';
          createSoundCheckbox();
          initEmulator(rom, Module, canvas);
        }
      };
      reader.readAsArrayBuffer(file);
    }
  });
}

/**
 * Main application startup.
 */
async function startApp() {
  const Module = (window as any).Module;
  if (!Module) {
    console.error("Chip8 Module not found. Ensure chip8.js is loaded.");
    return;
  }

  // Initialize the Module.
  const startModule = () => Module._init();
  if (Module.calledRun) {
    startModule();
  } else {
    Module.onRuntimeInitialized = startModule;
  }

  const canvas = document.getElementById('glCanvas') as HTMLCanvasElement;
  if (!canvas) {
    console.error("Canvas element with id 'glCanvas' not found.");
    return;
  }

  // If no ROM is loaded, set up the file loader.
  if (!rom || rom.length === 0) {
    setupFileLoader(Module, canvas);
  } else {
    createSoundCheckbox();
    initEmulator(rom, Module, canvas);
  }
}

// Start the application once the DOM is ready.
document.addEventListener('DOMContentLoaded', startApp);