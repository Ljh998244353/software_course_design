"use client";

export const dynamic = "force-dynamic";

import { useParams } from "next/navigation";
import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { AnimatePresence, motion } from "framer-motion";
import { FormEvent, useMemo, useState } from "react";
import { AlertTriangle, Clock3, History, Loader2, Radio, ShieldCheck, Star, Zap } from "lucide-react";
import { OfflineBanner } from "@/components/layout/offline-banner";
import { SiteNav } from "@/components/layout/site-nav";
import { Button } from "@/components/ui/button";
import { Toast } from "@/components/ui/toast";
import { useAuth } from "@/lib/auth-context";
import { getAuction, getBids, placeBid } from "@/lib/api/client";
import { queryKeys } from "@/lib/query/keys";
import { useAuctionWebSocket } from "@/lib/realtime/use-auction-ws";
import { countdownLabel, formatPrice, formatTime } from "@/lib/utils/format";
import type { BidRecord } from "@/types/auction";

const tabs = ["描述", "参数", "卖家评价"] as const;

export default function AuctionDetailPage() {
  const params = useParams<{ id: string }>();
  const id = params.id;
  const queryClient = useQueryClient();
  const { user } = useAuth();
  const [amount, setAmount] = useState("");
  const [pendingBid, setPendingBid] = useState<BidRecord | null>(null);
  const [toast, setToast] = useState("");
  const [toastTone, setToastTone] = useState<"success" | "danger" | "info">("success");
  const [cooldown, setCooldown] = useState(0);
  const [shake, setShake] = useState(false);
  const [activeTab, setActiveTab] = useState<(typeof tabs)[number]>("描述");

  const auctionQuery = useQuery({ queryKey: queryKeys.auction(id), queryFn: () => getAuction(id) });
  const bidsQuery = useQuery({
    queryKey: queryKeys.bids(id),
    queryFn: () => getBids(id),
    refetchInterval: 4000,
  });
  const auction = auctionQuery.data;
  const bids = bidsQuery.data ?? [];
  const nextBid = useMemo(() => (auction ? auction.currentPrice + auction.minIncrement : 0), [auction]);

  const ws = useAuctionWebSocket({
    auctionId: id,
    enabled: true,
    onPriceUpdate: async () => {
      await queryClient.invalidateQueries({ queryKey: queryKeys.auction(id) });
      await queryClient.invalidateQueries({ queryKey: queryKeys.bids(id) });
    },
    onAuctionExtended: async () => {
      setToast("拍卖触发延时保护，结束时间已顺延");
      setToastTone("info");
      await queryClient.invalidateQueries({ queryKey: queryKeys.auction(id) });
    },
    onOutbid: async () => {
      setToast("你已被其他用户超价");
      setToastTone("danger");
      await queryClient.invalidateQueries({ queryKey: queryKeys.auction(id) });
    },
  });

  const mutation = useMutation({
    mutationFn: (bidAmount: number) => placeBid(id, bidAmount),
    onMutate: (bidAmount) => {
      const pending: BidRecord = {
        id: `PENDING-${Date.now()}`,
        auctionId: id,
        bidder: user?.nickname || user?.username || "当前用户",
        amount: bidAmount,
        createdAt: new Date().toISOString(),
        status: "PENDING",
      };
      setPendingBid(pending);
      setToast("出价已进入 pending，等待权威确认");
      setToastTone("info");
    },
    onSuccess: async () => {
      setPendingBid(null);
      setToast("出价成功，权威价格已刷新");
      setToastTone("success");
      await queryClient.invalidateQueries({ queryKey: queryKeys.bids(id) });
      await queryClient.invalidateQueries({ queryKey: queryKeys.auction(id) });
    },
    onError: (error) => {
      setPendingBid(null);
      const message = error instanceof Error ? error.message : "出价失败";
      setToastTone("danger");
      if (message.includes("409")) {
        setToast("价格已被其他用户抢先更新，已回滚本次乐观出价");
        setShake(true);
        setTimeout(() => setShake(false), 260);
      } else if (message.includes("429")) {
        setToast("请求过快，进入 10 秒冷却");
        setCooldown(10);
        const timer = setInterval(() => {
          setCooldown((value) => {
            if (value <= 1) {
              clearInterval(timer);
              return 0;
            }
            return value - 1;
          });
        }, 1000);
      } else {
        setToast(message);
      }
    },
  });

  function submitBid(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    if (!auction || cooldown > 0 || !auction.acceptingBids) {
      return;
    }
    mutation.mutate(Number(amount || nextBid));
  }

  if (!auction) {
    return (
      <>
        <SiteNav />
        <main className="mx-auto max-w-7xl px-4 py-10">加载拍卖详情...</main>
      </>
    );
  }

  const connectionLabel = ws.isConnected ? "实时通道已连接" : ws.isFallback ? "实时通道降级为轮询" : "正在建立实时连接";

  return (
    <>
      <SiteNav />
      {!ws.isConnected ? <OfflineBanner text="实时通道不可用时自动回退到短轮询，价格以数据库权威结果为准。" /> : null}
      <Toast message={toast} tone={toastTone} />
      <main className="mx-auto grid max-w-7xl gap-8 px-4 py-8 sm:px-6 lg:grid-cols-[1.2fr_0.8fr] lg:px-8">
        <section className="space-y-5">
          <div className="relative aspect-[16/10] overflow-hidden rounded-lg border border-slate-200 bg-white shadow-card">
            <img src={auction.imageUrl} alt={auction.title} className="h-full w-full object-cover" />
          </div>
          <div className="rounded-lg border border-slate-200 bg-white p-5 shadow-card">
            <div className="mb-4 flex gap-2 border-b border-slate-200 pb-4 text-sm font-bold text-slate-600">
              {tabs.map((tab) => (
                <button
                  key={tab}
                  onClick={() => setActiveTab(tab)}
                  className={`rounded-lg px-3 py-2 ${activeTab === tab ? "bg-indigo-50 text-indigo-700" : "hover:bg-slate-50"}`}
                >
                  {tab}
                </button>
              ))}
            </div>
            <h1 className="text-2xl font-black text-slate-950">{auction.title}</h1>
            {activeTab === "描述" ? (
              <>
                <p className="mt-3 leading-8 text-slate-600">{auction.description}</p>
                <div className="mt-5 grid gap-3 sm:grid-cols-3">
                  {auction.tags.map((tag) => (
                    <span key={tag} className="rounded-lg border border-slate-200 bg-slate-50 px-3 py-2 text-sm font-bold text-slate-700">
                      {tag}
                    </span>
                  ))}
                </div>
              </>
            ) : null}
            {activeTab === "参数" ? (
              <div className="mt-4 grid gap-3 sm:grid-cols-2">
                <DetailMeta label="分类" value={auction.category} />
                <DetailMeta label="交易方式" value={auction.tradeMode || "校园当面交易"} />
                <DetailMeta label="交付地点" value={auction.location} />
                <DetailMeta label="最小加价" value={formatPrice(auction.minIncrement)} />
              </div>
            ) : null}
            {activeTab === "卖家评价" ? (
              <div className="mt-4 grid gap-3 sm:grid-cols-2">
                <DetailMeta label="卖家账号" value={auction.seller.name} />
                <DetailMeta label="平均评分" value={`${auction.seller.rating.toFixed(1)} / 5`} />
                <DetailMeta label="成交记录" value={`${auction.seller.deals} 笔`} />
                <DetailMeta label="关注人数" value={`${auction.watchers} 人`} />
              </div>
            ) : null}
          </div>
        </section>

        <aside className={`sticky top-24 h-fit rounded-lg border border-slate-200 bg-white p-5 shadow-float ${shake ? "animate-[shake_0.26s]" : ""}`}>
          <div className="mb-5 flex items-center justify-between">
            <div>
              <p className="text-sm font-bold text-indigo-600">CURRENT PRICE</p>
              <p className="tabular mt-1 text-4xl font-black text-slate-950">{formatPrice(auction.currentPrice)}</p>
            </div>
            <span className="tabular flex items-center gap-1 rounded-full border border-amber-200 bg-amber-50 px-3 py-1 text-sm font-bold text-amber-700">
              <Clock3 className="h-4 w-4" />
              {countdownLabel(auction.endTime)}
            </span>
          </div>

          <div className="mb-5 rounded-lg border border-slate-200 bg-slate-50 p-4">
            <div className="flex items-center gap-2 text-sm font-bold text-slate-700">
              <Radio className={`h-4 w-4 ${ws.isConnected ? "text-emerald-600" : "text-amber-600"}`} />
              {connectionLabel}
            </div>
            <p className="mt-2 text-xs leading-5 text-slate-500">卖家 {auction.seller.name} · {auction.location}</p>
          </div>

          <div className="mb-4 flex items-center gap-2 rounded-lg border border-slate-200 bg-slate-50 px-3 py-2 text-sm text-slate-600">
            <Star className="h-4 w-4 text-amber-500" />
            卖家评分 {auction.seller.rating.toFixed(1)} · 成交 {auction.seller.deals} 笔
          </div>

          <form onSubmit={submitBid} className="space-y-4">
            <div className="grid grid-cols-3 gap-2">
              {[nextBid, nextBid + auction.minIncrement, nextBid + auction.minIncrement * 2].map((value) => (
                <button key={value} type="button" onClick={() => setAmount(String(value))} className="rounded-lg border border-slate-200 bg-white px-3 py-2 text-sm font-bold text-slate-700 hover:border-indigo-300 hover:text-indigo-700">
                  +{formatPrice(value - auction.currentPrice)}
                </button>
              ))}
            </div>
            <input value={amount} onChange={(event) => setAmount(event.target.value)} className="tabular h-12 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 text-lg font-bold outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-500/20" placeholder={String(nextBid)} inputMode="numeric" />
            <Button className="w-full" disabled={mutation.isPending || cooldown > 0 || !auction.acceptingBids}>
              {mutation.isPending ? <Loader2 className="h-4 w-4 animate-spin" /> : <Zap className="h-4 w-4" />}
              {cooldown > 0 ? `冷却 ${cooldown}s` : auction.acceptingBids ? "PLACE BID" : "拍卖不可出价"}
            </Button>
          </form>

          <div className="mt-6">
            <h2 className="mb-3 flex items-center gap-2 text-sm font-black text-slate-950">
              <History className="h-4 w-4 text-indigo-600" />
              出价历史
            </h2>
            <div className="space-y-2">
              <AnimatePresence>
                {pendingBid ? <BidRow key={pendingBid.id} bid={pendingBid} /> : null}
                {bids.map((bid) => (
                  <BidRow key={bid.id} bid={bid} />
                ))}
              </AnimatePresence>
            </div>
          </div>

          <AnimatePresence>
            {cooldown > 0 ? (
              <motion.div initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} className="absolute inset-0 flex items-center justify-center rounded-lg bg-white/78 backdrop-blur">
                <div className="text-center">
                  <AlertTriangle className="mx-auto mb-3 h-8 w-8 text-amber-600" />
                  <p className="tabular text-3xl font-black text-slate-950">{cooldown}s</p>
                  <p className="mt-1 text-sm font-bold text-slate-600">限流冷却中</p>
                </div>
              </motion.div>
            ) : null}
          </AnimatePresence>

          <div className="mt-5 flex items-center gap-2 border-t border-slate-100 pt-4 text-xs font-bold text-slate-500">
            <ShieldCheck className="h-4 w-4 text-emerald-600" />
            出价有效性以数据库事务提交为准
          </div>
        </aside>
      </main>
    </>
  );
}

function DetailMeta({ label, value }: { label: string; value: string }) {
  return (
    <div className="rounded-lg border border-slate-200 bg-slate-50 px-4 py-3">
      <p className="text-xs font-bold text-slate-500">{label}</p>
      <p className="mt-1 font-bold text-slate-950">{value}</p>
    </div>
  );
}

function BidRow({ bid }: { bid: BidRecord }) {
  const style =
    bid.status === "PENDING"
      ? "border-slate-200 bg-slate-50 text-slate-500"
      : bid.status === "SUCCESS"
        ? "border-emerald-200 bg-emerald-50 text-emerald-700"
        : "border-slate-200 bg-white text-slate-500";

  return (
    <motion.div layout initial={{ opacity: 0, y: -8 }} animate={{ opacity: 1, y: 0 }} exit={{ opacity: 0, y: -8 }} className={`flex items-center justify-between rounded-lg border px-3 py-2 text-sm ${style}`}>
      <span className="font-bold">{bid.bidder}</span>
      <span className="tabular font-black">{formatPrice(bid.amount)}</span>
      <span className="tabular text-xs">{formatTime(bid.createdAt)}</span>
    </motion.div>
  );
}
