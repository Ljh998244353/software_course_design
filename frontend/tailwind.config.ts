import type { Config } from "tailwindcss";

const config: Config = {
  content: [
    "./app/**/*.{js,ts,jsx,tsx,mdx}",
    "./components/**/*.{js,ts,jsx,tsx,mdx}",
    "./hooks/**/*.{js,ts,jsx,tsx,mdx}",
    "./lib/**/*.{js,ts,jsx,tsx,mdx}"
  ],
  theme: {
    extend: {
      colors: {
        surface: "#FFFFFF",
        ink: "#0F172A",
        muted: "#475569",
        line: "#E2E8F0",
        primary: "#4F46E5",
        success: "#10B981",
        warning: "#F59E0B",
        danger: "#E11D48"
      },
      boxShadow: {
        card: "0 1px 3px 0 rgba(0, 0, 0, 0.05)",
        float: "0 10px 15px -3px rgba(0, 0, 0, 0.05), 0 4px 6px -4px rgba(0, 0, 0, 0.05)",
        modal: "0 25px 50px -12px rgba(0, 0, 0, 0.08)"
      },
      transitionTimingFunction: {
        auction: "cubic-bezier(0.16, 1, 0.3, 1)"
      }
    }
  },
  plugins: []
};

export default config;
