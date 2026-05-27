import { ButtonHTMLAttributes, ReactNode } from "react";

type ButtonProps = ButtonHTMLAttributes<HTMLButtonElement> & {
  variant?: "primary" | "secondary" | "danger" | "ghost";
  children: ReactNode;
};

const variants = {
  primary: "bg-indigo-600 text-white hover:bg-indigo-700 shadow-float",
  secondary: "bg-white text-slate-900 border border-slate-200 hover:border-indigo-300 hover:text-indigo-700",
  danger: "bg-rose-600 text-white hover:bg-rose-700 shadow-float",
  ghost: "text-slate-600 hover:text-indigo-700 hover:bg-indigo-50"
};

export function Button({ variant = "primary", className = "", children, ...props }: ButtonProps) {
  return (
    <button
      className={`auction-transition inline-flex min-h-10 items-center justify-center gap-2 rounded-lg px-4 py-2 text-sm font-semibold disabled:cursor-not-allowed disabled:opacity-50 ${variants[variant]} ${className}`}
      {...props}
    >
      {children}
    </button>
  );
}
