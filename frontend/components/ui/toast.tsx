"use client";

import { CheckCircle2, XCircle } from "lucide-react";
import { AnimatePresence, motion } from "framer-motion";

type ToastProps = {
  message: string;
  tone?: "success" | "danger" | "info";
};

export function Toast({ message, tone = "success" }: ToastProps) {
  const Icon = tone === "danger" ? XCircle : CheckCircle2;
  const styles =
    tone === "danger"
      ? "border-rose-200 bg-rose-50 text-rose-700"
      : tone === "info"
        ? "border-indigo-200 bg-indigo-50 text-indigo-700"
        : "border-emerald-200 bg-emerald-50 text-emerald-700";

  return (
    <AnimatePresence>
      {message ? (
        <motion.div
          initial={{ opacity: 0, y: -12 }}
          animate={{ opacity: 1, y: 0 }}
          exit={{ opacity: 0, y: -12 }}
          className={`fixed right-6 top-6 z-50 flex items-center gap-3 rounded-xl border px-4 py-3 text-sm font-semibold shadow-modal backdrop-blur ${styles}`}
          role="status"
          aria-live="polite"
        >
          <Icon className="h-5 w-5" />
          {message}
        </motion.div>
      ) : null}
    </AnimatePresence>
  );
}
