/** @type {import('tailwindcss').Config} */
export default {
  content: ['./src/**/*.{html,js,svelte,ts}', './index.html'],
  theme: {
    extend: {
      colors: {
        'hypr-blue': '#00D9FF',
        'hypr-pink': '#FF007F',
        'hypr-purple': '#9D00FF',
        'hypr-dark': '#0A0E1B',
        'hypr-darker': '#050810',
      },
      fontFamily: {
        'mono': ['JetBrains Mono', 'monospace'],
      },
      animation: {
        'float': 'float 6s ease-in-out infinite',
      },
      keyframes: {
        float: {
          '0%, 100%': { transform: 'translateY(0px)' },
          '50%': { transform: 'translateY(-20px)' }
        }
      }
    },
  },
  plugins: [],
}