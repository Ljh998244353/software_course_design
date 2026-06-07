"use client";

import Link from "next/link";
import { useRouter } from "next/navigation";
import { Gavel, LogOut, Search, User } from "lucide-react";
import { FormEvent, useState } from "react";
import { useAuth } from "@/lib/auth-context";

export function SiteNav() {
  const { user, loading, logout } = useAuth();
  const router = useRouter();
  const [keyword, setKeyword] = useState("");

  function submitSearch(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    const params = new URLSearchParams();
    if (keyword.trim()) {
      params.set("keyword", keyword.trim());
    }
    router.push(`/auction/hall${params.toString() ? `?${params.toString()}` : ""}`);
  }

  return (
    <header className="sticky top-0 z-40 border-b border-slate-200 bg-white/92 backdrop-blur">
      <div className="mx-auto flex h-16 max-w-7xl items-center gap-5 px-4 sm:px-6 lg:px-8">
        <Link href="/" className="flex items-center gap-2 text-base font-bold text-slate-950">
          <span className="flex h-9 w-9 items-center justify-center rounded-lg bg-indigo-600 text-white">
            <Gavel className="h-5 w-5" />
          </span>
          校园拍卖
        </Link>
        <form onSubmit={submitSearch} className="hidden h-10 w-[400px] max-w-full items-center gap-2 rounded-lg border border-slate-200 bg-slate-50 px-3 text-sm text-slate-500 md:flex">
          <Search className="h-4 w-4" />
          <input
            value={keyword}
            onChange={(event) => setKeyword(event.target.value)}
            placeholder="搜索拍品、分类、卖家"
            className="h-full flex-1 bg-transparent outline-none placeholder:text-slate-400"
          />
        </form>
        <nav className="ml-auto hidden items-center gap-2 md:flex">
          <Link className="rounded-lg px-3 py-2 text-sm font-medium text-slate-600 hover:bg-indigo-50 hover:text-indigo-700" href="/auction/hall">
            拍卖大厅
          </Link>
          {user && (
            <Link className="rounded-lg px-3 py-2 text-sm font-medium text-slate-600 hover:bg-indigo-50 hover:text-indigo-700" href="/auction/publish">
              发布拍品
            </Link>
          )}
          {user?.role_code === "ADMIN" && (
            <Link className="rounded-lg px-3 py-2 text-sm font-medium text-slate-600 hover:bg-indigo-50 hover:text-indigo-700" href="/admin/dashboard">
              管理大盘
            </Link>
          )}
          {loading && user ? (
            <div className="h-10 w-20 animate-pulse rounded-lg bg-slate-100" />
          ) : user ? (
            <div className="flex items-center gap-2">
              <span className="inline-flex h-10 items-center gap-1.5 rounded-lg px-3 text-sm font-medium text-slate-700">
                <User className="h-4 w-4" />
                {user.nickname}
              </span>
              <button
                onClick={() => logout()}
                className="inline-flex h-10 items-center justify-center rounded-lg border border-slate-200 bg-white px-3 text-sm font-medium text-slate-600 hover:border-red-300 hover:text-red-600"
              >
                <LogOut className="h-4 w-4" />
              </button>
            </div>
          ) : (
            <Link
              href="/auth/login"
              className="auction-transition inline-flex h-10 items-center justify-center rounded-lg border border-slate-200 bg-white px-4 text-sm font-semibold text-slate-900 hover:border-indigo-300 hover:text-indigo-700"
            >
              登录/注册
            </Link>
          )}
        </nav>
      </div>
    </header>
  );
}
