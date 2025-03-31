import { defineConfig } from 'vite'
import { resolve } from 'path'

export default defineConfig({
  build: {
    rollupOptions: {
      input: {
        // Default entry (if you have one)
        main: resolve(__dirname, 'index.html'),
        chip8: resolve(__dirname, 'chip8.html'),
        atari2600: resolve(__dirname, 'atari2600.html')
      }
    }
  }
})