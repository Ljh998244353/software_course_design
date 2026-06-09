"use client";

export const dynamic = "force-dynamic";

import { useQuery } from "@tanstack/react-query";
import { Activity, BellRing, CircleAlert, RotateCcw, SlidersHorizontal } from "lucide-react";
import { usePathname, useRouter, useSearchParams } from "next/navigation";
import { Suspense } from "react";
import Link from "next/link";
import { AuctionCard } from "@/components/auction/auction-card";
import { SiteNav } from "@/components/layout/site-nav";
import { OfflineBanner } from "@/components/layout/offline-banner";
import { Button } from "@/components/ui/button";
import { Skeleton } from "@/components/ui/skeleton";
import { getAuctions } from "@/lib/api/client";
import { useAuth } from "@/lib/auth-context";
import { queryKeys } from "@/lib/query/keys";
import { formatPrice } from "@/lib/utils/format";
import type { AuctionListQuery } from "@/types/auction";

const categories = ["数码设备", "图书文创", "运动户外", "校园收藏"];
const tradeModes = [
  { label: "校园当面交易", value: "MEETUP" },
  { label: "自提", value: "SELF_PICKUP" },
  { label: "可邮寄", value: "SHIPPING" },
];

export default function AuctionHallPage() {
  return (
    <Suspense fallback={<HallFallback />}>
      <AuctionHallContent />
    </Suspense>
  );
}

function AuctionHallContent() {
  const router = useRouter();
  const pathname = usePathname();
  const searchParams = useSearchParams();
  const { user, loading: authLoading } = useAuth();

  const query: AuctionListQuery = {
    keyword: searchParams.get("keyword") || undefined,
    category: searchParams.get("category") || undefined,
    status: searchParams.get("status") || "RUNNING",
    priceMin: searchParams.get("price_min") ? Number(searchParams.get("price_min")) : undefined,
    priceMax: searchParams.get("price_max") ? Number(searchParams.get("price_max")) : undefined,
    sellerRating: searchParams.get("seller_rating") ? Number(searchParams.get("seller_rating")) : undefined,
    sellerHasDeals: searchParams.get("seller_has_deals") === "true" ? true : undefined,
    tradeMode: searchParams.get("trade_mode") || undefined,
    pageNo: 1,
    pageSize: 24,
  };

  const { data: auctions = [], error, isLoading, isFetching, refetch } = useQuery({
    queryKey: queryKeys.auctions(query),
    queryFn: () => getAuctions(query),
    refetchInterval: 5000,
  });
  const hasFilters = Boolean(
    query.keyword ||
      query.category ||
      query.priceMin !== undefined ||
      query.priceMax !== undefined ||
      query.sellerRating !== undefined ||
      query.sellerHasDeals ||
      query.tradeMode
  );
  const isInitialLoading = isLoading && auctions.length === 0;

  function updateParam(key: string, value?: string) {
    const params = new URLSearchParams(searchParams.toString());
    if (!value) {
      params.delete(key);
    } else {
      params.set(key, value);
    }
    router.replace(`${pathname}?${params.toString()}`);
  }

  function resetFilters() {
    router.replace(pathname);
  }

  return (
    <>
      <SiteNav />
      <OfflineBanner text="大厅使用 5 秒短轮询刷新，详情页优先使用 WebSocket 实时通道。" />
      <main className="mx-auto grid max-w-7xl gap-6 px-4 py-8 sm:px-6 lg:grid-cols-[260px_1fr] lg:px-8">
        <aside className="h-fit rounded-lg border border-slate-200 bg-white p-5 shadow-card">
          <div className="mb-5 flex items-center justify-between gap-2">
            <div className="flex items-center gap-2">
              <SlidersHorizontal className="h-5 w-5 text-indigo-600" />
              <h2 className="text-base font-black text-slate-950">筛选</h2>
            </div>
            <button onClick={resetFilters} className="inline-flex items-center gap-1 text-xs font-bold text-slate-500 hover:text-indigo-700">
              <RotateCcw className="h-3.5 w-3.5" />
              重置
            </button>
          </div>
          <div className="space-y-5 text-sm">
            <div>
              <p className="mb-3 font-bold text-slate-700">分类</p>
              <div className="flex flex-wrap gap-2">
                {categories.map((category) => (
                  <button
                    key={category}
                    onClick={() => updateParam("category", query.category === category ? undefined : category)}
                    className={`rounded-full px-3 py-1.5 text-xs font-bold ${query.category === category ? "bg-indigo-50 text-indigo-700" : "bg-slate-50 text-slate-600 hover:bg-slate-100"}`}
                  >
                    {category}
                  </button>
                ))}
              </div>
            </div>
            <div>
              <p className="mb-3 font-bold text-slate-700">价格区间</p>
              <div className="grid grid-cols-2 gap-2">
                <input
                  key={`price-min-${query.priceMin ?? ""}`}
                  defaultValue={query.priceMin ?? ""}
                  onBlur={(event) => updateParam("price_min", event.target.value.trim() || undefined)}
                  className="h-10 rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500"
                  placeholder="最低"
                />
                <input
                  key={`price-max-${query.priceMax ?? ""}`}
                  defaultValue={query.priceMax ?? ""}
                  onBlur={(event) => updateParam("price_max", event.target.value.trim() || undefined)}
                  className="h-10 rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500"
                  placeholder="最高"
                />
              </div>
            </div>
            <div>
              <p className="mb-3 font-bold text-slate-700">卖家信用</p>
              <div className="space-y-2">
                {[
                  { label: "4.8 以上", value: "4.8" },
                  { label: "4.5 以上", value: "4.5" },
                ].map((item) => (
                  <label key={item.value} className="flex items-center gap-2 text-slate-600">
                    <input
                      type="radio"
                      checked={String(query.sellerRating ?? "") === item.value}
                      onChange={() => updateParam("seller_rating", item.value)}
                      className="h-4 w-4 border-slate-300 text-indigo-600"
                    />
                    {item.label}
                  </label>
                ))}
                <label className="flex items-center gap-2 text-slate-600">
                  <input
                    type="checkbox"
                    checked={Boolean(query.sellerHasDeals)}
                    onChange={(event) => updateParam("seller_has_deals", event.target.checked ? "true" : undefined)}
                    className="h-4 w-4 rounded border-slate-300 text-indigo-600"
                  />
                  有成交记录
                </label>
              </div>
            </div>
            <div>
              <p className="mb-3 font-bold text-slate-700">交易方式</p>
              <div className="space-y-2">
                {tradeModes.map((item) => (
                  <label key={item.value} className="flex items-center gap-2 text-slate-600">
                    <input
                      type="radio"
                      checked={query.tradeMode === item.value}
                      onChange={() => updateParam("trade_mode", item.value)}
                      className="h-4 w-4 border-slate-300 text-indigo-600"
                    />
                    {item.label}
                  </label>
                ))}
              </div>
            </div>
          </div>
        </aside>

        <section>
          <div className="mb-6 flex flex-col gap-4 rounded-lg border border-slate-200 bg-white p-5 shadow-card md:flex-row md:items-center md:justify-between">
            <div>
              <p className="text-sm font-bold text-indigo-600">AUCTION HALL</p>
              <h1 className="text-2xl font-black text-slate-950">拍卖大厅</h1>
              <p className="mt-1 text-sm text-slate-500">当前结果 {auctions.length} 条</p>
              {isFetching && !isInitialLoading ? <p className="mt-1 text-xs font-medium text-slate-400">数据刷新中...</p> : null}
            </div>
            <div className="rounded-lg border border-slate-200 bg-slate-950 px-4 py-3 text-white">
              <div className="flex items-center gap-2 text-xs font-bold text-emerald-300">
                <Activity className="h-4 w-4" />
                Mini Live Ticker
              </div>
              <p className="mt-1 text-sm">
                <BellRing className="mr-1 inline h-4 w-4" />
                {auctions[0]?.title ?? "暂无实时拍卖"} 当前价 {auctions[0] ? formatPrice(auctions[0].currentPrice) : "-"}
              </p>
            </div>
          </div>

          {isInitialLoading ? (
            <div className="grid gap-5 md:grid-cols-2 xl:grid-cols-3">
              {Array.from({ length: 6 }).map((_, index) => (
                <Skeleton key={index} className="h-[360px]" />
              ))}
            </div>
          ) : error ? (
            <div className="rounded-lg border border-rose-200 bg-rose-50 p-8 text-center shadow-card">
              <div className="mx-auto flex h-12 w-12 items-center justify-center rounded-full bg-white text-rose-600 shadow-card">
                <CircleAlert className="h-6 w-6" />
              </div>
              <h2 className="mt-4 text-xl font-black text-slate-950">大厅数据加载失败</h2>
              <p className="mt-2 text-sm leading-6 text-slate-600">{error instanceof Error ? error.message : "请检查后端服务或接口状态后重试。"}</p>
              <div className="mt-5 flex justify-center gap-3">
                <Button onClick={() => refetch()}>重新加载</Button>
                <Button variant="secondary" onClick={resetFilters}>清空筛选</Button>
              </div>
            </div>
          ) : auctions.length === 0 ? (
            <div className="rounded-lg border border-slate-200 bg-white p-10 text-center shadow-card">
              <div className="mx-auto flex h-14 w-14 items-center justify-center rounded-full bg-slate-100 text-slate-500">
                <BellRing className="h-6 w-6" />
              </div>
              <h2 className="mt-4 text-2xl font-black text-slate-950">{hasFilters ? "没有匹配的拍品" : "当前还没有在拍拍品"}</h2>
              <p className="mt-2 text-sm leading-6 text-slate-600">
                {hasFilters
                  ? "调整筛选条件后会自动重新查询。"
                  : user
                    ? "管理员创建拍卖后会在这里展示。你可以先发布拍品、查看通知，或稍后刷新。"
                    : "管理员创建拍卖后会在这里展示。现在可以先登录、发布拍品，或稍后刷新查看。"}
              </p>
              <div className="mt-6 flex flex-wrap justify-center gap-3">
                {hasFilters ? (
                  <Button onClick={resetFilters}>清空筛选</Button>
                ) : authLoading ? null : user ? (
                  <>
                    <Link href="/auction/publish" className="auction-transition inline-flex min-h-10 items-center justify-center rounded-lg bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-float hover:bg-indigo-700">
                      发布拍品
                    </Link>
                    <Link href="/notifications" className="auction-transition inline-flex min-h-10 items-center justify-center rounded-lg border border-slate-200 bg-white px-4 py-2 text-sm font-semibold text-slate-900 hover:border-indigo-300 hover:text-indigo-700">
                      查看通知
                    </Link>
                  </>
                ) : (
                  <Link href="/auth/login" className="auction-transition inline-flex min-h-10 items-center justify-center rounded-lg bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-float hover:bg-indigo-700">登录 / 注册</Link>
                )}
                <Button variant="secondary" onClick={() => refetch()}>刷新列表</Button>
              </div>
            </div>
          ) : (
            <div className="grid gap-5 md:grid-cols-2 xl:grid-cols-3">
              {auctions.map((auction) => (
                <AuctionCard key={auction.id} auction={auction} />
              ))}
            </div>
          )}
        </section>
      </main>
    </>
  );
}

function HallFallback() {
  return (
    <>
      <SiteNav />
      <main className="mx-auto max-w-7xl px-4 py-8 sm:px-6 lg:px-8">
        <div className="grid gap-5 md:grid-cols-2 xl:grid-cols-3">
          {Array.from({ length: 6 }).map((_, index) => (
            <Skeleton key={index} className="h-[360px]" />
          ))}
        </div>
      </main>
    </>
  );
}
