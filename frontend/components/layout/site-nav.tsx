import Link from "next/link";
import { Gavel, Search } from "lucide-react";

export function SiteNav() {
  return (
    <header className="sticky top-0 z-40 border-b border-slate-200 bg-white/92 backdrop-blur">
      <div className="mx-auto flex h-16 max-w-7xl items-center gap-5 px-4 sm:px-6 lg:px-8">
        <Link href="/" className="flex items-center gap-2 text-base font-bold text-slate-950">
          <span className="flex h-9 w-9 items-center justify-center rounded-lg bg-indigo-600 text-white">
            <Gavel className="h-5 w-5" />
          </span>
          校园拍卖
        </Link>
        <div className="hidden h-10 w-[400px] max-w-full items-center gap-2 rounded-lg border border-slate-200 bg-slate-50 px-3 text-sm text-slate-500 md:flex">
          <Search className="h-4 w-4" />
          搜索拍品、分类、卖家
        </div>
        <nav className="ml-auto hidden items-center gap-2 md:flex">
          <Link className="rounded-lg px-3 py-2 text-sm font-medium text-slate-600 hover:bg-indigo-50 hover:text-indigo-700" href="/auction/hall">
            拍卖大厅
          </Link>
          <Link className="rounded-lg px-3 py-2 text-sm font-medium text-slate-600 hover:bg-indigo-50 hover:text-indigo-700" href="/auction/publish">
            发布拍品
          </Link>
          <Link className="rounded-lg px-3 py-2 text-sm font-medium text-slate-600 hover:bg-indigo-50 hover:text-indigo-700" href="/admin/dashboard">
            管理大盘
          </Link>
          <Link
            href="/auth/login"
            className="auction-transition inline-flex h-10 items-center justify-center rounded-lg border border-slate-200 bg-white px-4 text-sm font-semibold text-slate-900 hover:border-indigo-300 hover:text-indigo-700"
          >
            登录/注册
          </Link>
        </nav>
      </div>
    </header>
  );
}
