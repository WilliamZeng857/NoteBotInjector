import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// https://vite.dev/config/
export default defineConfig({
  plugins: [vue()],
  server: {
    host: '127.0.0.1',
    port: 30188,
    proxy: {
      '/api': {
        target: 'http://127.0.0.1:30186',
        changeOrigin: true,
      },
    },
  },
})
