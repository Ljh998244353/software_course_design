"use client";

import { useQuery } from "@tanstack/react-query";
import { Activity, BellRing, SlidersHorizontal } from "lucide-react";
import { AuctionCard } from "@/components/auction/auction-card";
import { SiteNav } from "@/components/layout/site-nav";
import { OfflineBanner } from "@/components/layout/offline-banner";
import { Skeleton } from "@/components/ui/skeleton";
import { getAuctions } from "@/lib/api/client";
import { queryKeys } from "@/lib/query/keys";
import { formatPrice } from "@/lib/utils/format";

export default function AuctionHallPage() {
  const { data: auctions = [], isLoading } = useQuery({
    queryKey: queryKeys.auctions,
    queryFn: getAuctions,
    refetchInterval: 5000
  });

  return (
    <>
      <SiteNav />
      <OfflineBanner text="大厅使用 5 秒短轮询刷新，详情页才建立单拍卖实时通道。" />
      <main className="mx-auto grid max-w-7xl gap-6 px-4 py-8 sm:px-6 lg:grid-cols-[260px_1fr] lg:px-8">
        <aside className="h-fit rounded-lg border border-slate-200 bg-white p-5 shadow-card">
          <div className="mb-5 flex items-center gap-2">
            <SlidersHorizontal className="h-5 w-5 text-indigo-600" />
            <h2 className="text-base font-black text-slate-950">筛选</h2>
          </div>
          <div className="space-y-5 text-sm">
            <div>
              <p className="mb-3 font-bold text-slate-700">价格区间</p>
              <div className="grid grid-cols-2 gap-2">
                <input className="h-10 rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500" placeholder="最低" />
                <input className="h-10 rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500" placeholder="最高" />
              </div>
            </div>
            <div>
              <p className="mb-3 font-bold text-slate-700">卖家信用</p>
              {["4.8 以上", "4.5 以上", "有成交记录"].map((item) => (
                <label key={item} className="mb-2 flex items-center gap-2 text-slate-600">
                  <input type="checkbox" className="h-4 w-4 rounded border-slate-300 text-indigo-600" />
                  {item}
                </label>
              ))}
            </div>
            <div>
              <p className="mb-3 font-bold text-slate-700">交易方式</p>
              {["校园自提", "当面验货", "可邮寄"].map((item) => (
                <label key={item} className="mb-2 flex items-center gap-2 text-slate-600">
                  <input type="checkbox" className="h-4 w-4 rounded border-slate-300 text-indigo-600" />
                  {item}
                </label>
              ))}
            </div>
          </div>
        </aside>

        <section>
          <div className="mb-6 flex flex-col gap-4 rounded-lg border border-slate-200 bg-white p-5 shadow-card md:flex-row md:items-center md:justify-between">
            <div>
              <p className="text-sm font-bold text-indigo-600">AUCTION HALL</p>
              <h1 className="text-2xl font-black text-slate-950">拍卖大厅</h1>
            </div>
            <div className="rounded-lg border border-slate-200 bg-slate-950 px-4 py-3 text-white">
              <div className="flex items-center gap-2 text-xs font-bold text-emerald-300">
                <Activity className="h-4 w-4" />
                Mini Live Ticker
              </div>
              <p className="mt-1 text-sm">
                <BellRing className="mr-1 inline h-4 w-4" />
                {auctions[0]?.title ?? "加载中"} 当前价 {auctions[0] ? formatPrice(auctions[0].currentPrice) : "-"}
              </p>
            </div>
          </div>

          {isLoading ? (
            <div className="grid gap-5 md:grid-cols-2 xl:grid-cols-3">
              {Array.from({ length: 6 }).map((_, index) => (
                <Skeleton key={index} className="h-[360px]" />
              ))}
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
