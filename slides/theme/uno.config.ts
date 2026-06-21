import { defineConfig, presetUno, presetIcons, presetWebFonts } from 'unocss'

export default defineConfig({
  shortcuts: {
    'bg-main': 'bg-[#fcfcfa] text-[#3f4654] dark:(bg-[#1a1b26] text-[#a9b1d6])',
    'gradient-text': 'text-[#1e3a5f]',
  },
  theme: {
    colors: {
      primary: {
        DEFAULT: '#1e3a5f',
        navy: '#1e3a5f',
        light: '#2c4a7c',
        ink: '#1a1f2b',
      }
    }
  },
  presets: [
    presetUno(),
    presetIcons(),
    presetWebFonts({
      fonts: {
        sans: 'Inter',
        serif: 'Source Serif 4',
        mono: 'JetBrains Mono'
      }
    })
  ]
})