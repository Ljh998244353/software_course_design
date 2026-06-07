"use client";

export const dynamic = "force-dynamic";

import Link from "next/link";
import { useQuery } from "@tanstack/react-query";
import { ArrowRight, BadgeCheck, BellRing, Clock3, ShieldCheck } from "lucide-react";
import { AuctionCard } from "@/components/auction/auction-card";
import { SiteNav } from "@/components/layout/site-nav";
import { Skeleton } from "@/components/ui/skeleton";
import { getAuctions } from "@/lib/api/client";
import { queryKeys } from "@/lib/query/keys";
import { formatPrice } from "@/lib/utils/format";

export default function HomePage() {
  const { data: auctions = [], isLoading } = useQuery({
    queryKey: queryKeys.auctions({ pageSize: 8 }),
    queryFn: () => getAuctions({ pageSize: 8 }),
  });

  const featured = auctions[1] ?? auctions[0];

  return (
    <>
      <SiteNav />
      <main>
        <section className="mx-auto grid min-h-[calc(50vh)] max-w-7xl gap-8 px-4 py-10 sm:px-6 lg:grid-cols-[1.05fr_0.95fr] lg:px-8">
          <div className="flex flex-col justify-center">
            <div className="mb-5 inline-flex w-fit items-center gap-2 rounded-full border border-indigo-200 bg-indigo-50 px-3 py-1 text-sm font-bold text-indigo-700">
              <ShieldCheck className="h-4 w-4" />
              MySQL 权威结算 · 实时竞价工作台
            </div>
            <h1 className="max-w-3xl text-4xl font-black leading-tight text-slate-950 sm:text-5xl">
              校园闲置进入可追踪的实时拍卖流程
            </h1>
            <p className="mt-5 max-w-2xl text-lg leading-8 text-slate-600">
              从拍品审核、实时竞价到订单支付，前端以清晰的状态反馈承接后端强一致业务契约。
            </p>
            <div className="mt-8 flex flex-wrap gap-3">
              <Link href="/auction/hall" className="auction-transition inline-flex items-center gap-2 rounded-lg bg-indigo-600 px-5 py-3 text-sm font-bold text-white shadow-float hover:bg-indigo-700">
                进入拍卖大厅
                <ArrowRight className="h-4 w-4" />
              </Link>
              <Link href="/auction/publish" className="auction-transition inline-flex items-center gap-2 rounded-lg border border-slate-200 bg-white px-5 py-3 text-sm font-bold text-slate-900 hover:border-indigo-300 hover:text-indigo-700">
                发布拍品
              </Link>
            </div>
          </div>
          {featured ? (
            <Link href={`/auction/detail/${featured.id}`} className="auction-transition relative overflow-hidden rounded-lg border border-slate-200 bg-white p-4 shadow-float hover:-translate-y-1 hover:border-indigo-300">
              <div className="aspect-[16/10] rounded-lg bg-cover bg-center" style={{ backgroundImage: `url(${featured.imageUrl})` }} />
              <div className="mt-5 flex items-start justify-between gap-4">
                <div>
                  <p className="text-sm font-semibold text-indigo-600">即将截标</p>
                  <h2 className="mt-1 text-2xl font-black text-slate-950">{featured.title}</h2>
                </div>
                <div className="rounded-lg bg-slate-50 px-4 py-3 text-right">
                  <p className="text-xs font-medium text-slate-500">当前价</p>
                  <p className="tabular text-2xl font-black text-slate-950">{formatPrice(featured.currentPrice)}</p>
                </div>
              </div>
              <div className="mt-5 grid grid-cols-3 gap-3 text-sm">
                {[
                  { title: "延时保护", desc: "临近截标自动顺延", icon: Clock3 },
                  { title: "审核准入", desc: "管理员审核后上架", icon: BadgeCheck },
                  { title: "竞价一致", desc: "最高价以事务确认为准", icon: ShieldCheck }
                ].map(({ title, desc, icon: Icon }) => (
                  <div key={title} className="rounded-lg border border-slate-200 bg-slate-50 p-3">
                    <Icon className="mb-2 h-4 w-4 text-indigo-600" />
                    <p className="font-bold text-slate-900">{title}</p>
                    <p className="mt-1 text-xs leading-5 text-slate-500">{desc}</p>
                  </div>
                ))}
              </div>
            </Link>
          ) : (
            <Skeleton className="h-[420px]" />
          )}
        </section>

        <section className="border-y border-slate-200 bg-white/72">
          <div className="mx-auto flex max-w-7xl gap-3 overflow-x-auto px-4 py-4 sm:px-6 lg:px-8">
            {["数码3C", "运动装备", "图书教材", "生活闲置"].map((category) => (
              <Link
                key={category}
                href={`/auction/hall?category=${encodeURIComponent(category)}`}
                className="auction-transition whitespace-nowrap rounded-full border border-slate-200 bg-white px-4 py-2 text-sm font-bold text-slate-700 hover:border-indigo-300 hover:bg-indigo-50 hover:text-indigo-700"
              >
                {category}
              </Link>
            ))}
          </div>
        </section>

        <section className="mx-auto max-w-7xl px-4 py-10 sm:px-6 lg:px-8">
          <div className="mb-6 flex items-end justify-between gap-4">
            <div>
              <p className="text-sm font-bold text-indigo-600">LIVE AUCTIONS</p>
              <h2 className="mt-1 text-2xl font-black text-slate-950">正在竞价</h2>
            </div>
            <Link href="/auction/hall" className="text-sm font-bold text-indigo-600 hover:text-indigo-700">
              查看全部
            </Link>
          </div>
          <div className="grid gap-5 sm:grid-cols-2 lg:grid-cols-4">
            {isLoading
              ? Array.from({ length: 4 }).map((_, index) => <Skeleton key={index} className="h-[360px]" />)
              : auctions.length > 0 ? (
                  auctions.slice(0, 4).map((auction) => <AuctionCard key={auction.id} auction={auction} />)
                ) : (
                  <div className="col-span-full rounded-lg border border-slate-200 bg-white p-10 text-center shadow-card">
                    <div className="mx-auto flex h-14 w-14 items-center justify-center rounded-full bg-slate-100 text-slate-500">
                      <BellRing className="h-6 w-6" />
                    </div>
                    <h3 className="mt-4 text-xl font-black text-slate-950">暂时没有正在竞价的拍品</h3>
                    <p className="mt-2 text-sm leading-6 text-slate-600">可以先登录发布拍品，或等待管理员创建新的拍卖活动。</p>
                    <div className="mt-5 flex justify-center gap-3">
                      <Link href="/auth/login" className="auction-transition inline-flex min-h-10 items-center justify-center rounded-lg bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-float hover:bg-indigo-700">
                        登录 / 注册
                      </Link>
                      <Link href="/auction/publish" className="auction-transition inline-flex min-h-10 items-center justify-center rounded-lg border border-slate-200 bg-white px-4 py-2 text-sm font-semibold text-slate-900 hover:border-indigo-300 hover:text-indigo-700">
                        发布拍品
                      </Link>
                    </div>
                  </div>
                )}
          </div>
        </section>
      </main>
    </>
  );
}
