import type { Metadata } from "next";
import { ReactNode } from "react";
import { Providers } from "@/app/providers";
import "@/app/globals.css";

export const metadata: Metadata = {
  title: "校园拍卖平台",
  description: "面向校园和小型社区的在线拍卖系统前端"
};

export default function RootLayout({ children }: { children: ReactNode }) {
  return (
    <html lang="zh-CN">
      <body>
        <Providers>{children}</Providers>
      </body>
    </html>
  );
}
