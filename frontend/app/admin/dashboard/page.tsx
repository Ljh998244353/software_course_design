"use client";

export const dynamic = "force-dynamic";

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { AnimatePresence, motion } from "framer-motion";
import { AlertCircle, BarChart3, Check, ChevronRight, ClipboardCheck, Command, Gavel, Loader2, ShieldAlert, X } from "lucide-react";
import { useMemo, useState } from "react";
import type { ReactNode } from "react";
import { Button } from "@/components/ui/button";
import { Skeleton } from "@/components/ui/skeleton";
import { approveItem, getAdminAuctions, getAdminDailyStatistics, getAdminReviews, rejectItem } from "@/lib/api/client";
import { queryKeys } from "@/lib/query/keys";
import { formatPrice, formatTime } from "@/lib/utils/format";
import type { AdminReviewItem, AuctionItem } from "@/types/auction";

type Panel = "reviews" | "auctions" | "stats" | "ops";

export default function AdminDashboardPage() {
  const queryClient = useQueryClient();
  const [panel, setPanel] = useState<Panel>("reviews");
  const [selected, setSelected] = useState<AdminReviewItem | null>(null);
  const [removed, setRemoved] = useState<string[]>([]);
  const [rejectReason, setRejectReason] = useState("不符合平台审核要求");
  const today = new Date().toISOString().slice(0, 10);
  const { data = [], isLoading } = useQuery({ queryKey: queryKeys.adminReviews, queryFn: getAdminReviews });
  const { data: auctions = [] } = useQuery({
    queryKey: queryKeys.adminAuctions("ALL"),
    queryFn: () => getAdminAuctions("ALL"),
  });
  const { data: dailyStatistics = [] } = useQuery({
    queryKey: queryKeys.adminDailyStatistics(today, today),
    queryFn: () => getAdminDailyStatistics(today, today),
  });
  const rows = data.filter((row) => !removed.includes(row.id));
  const todayStats = dailyStatistics[0];
  const approveMutation = useMutation({
    mutationFn: approveItem,
    onSuccess: (_result, id) => finishAudit(id),
  });
  const rejectMutation = useMutation({
    mutationFn: ({ id, reason }: { id: string; reason: string }) => rejectItem(id, reason),
    onSuccess: (_result, variables) => finishAudit(variables.id),
  });
  const auditError = approveMutation.error?.message ?? rejectMutation.error?.message;
  const isAuditing = approveMutation.isPending || rejectMutation.isPending;
  const soldCount = todayStats?.sold_count ?? 0;
  const unsoldCount = todayStats?.unsold_count ?? 0;
  const runningAuctions = useMemo(() => auctions.filter((auction) => auction.status === "RUNNING"), [auctions]);

  function finishAudit(id: string) {
    setRemoved((current) => [...current, id]);
    setSelected(null);
    setRejectReason("不符合平台审核要求");
    void queryClient.invalidateQueries({ queryKey: queryKeys.adminReviews });
  }

  return (
    <main className="grid min-h-screen bg-slate-100 lg:grid-cols-[240px_1fr]">
      <aside className="hidden bg-slate-950 p-5 text-white lg:block">
        <div className="mb-10 flex items-center gap-3 text-lg font-black">
          <span className="flex h-10 w-10 items-center justify-center rounded-lg bg-indigo-500">
            <Gavel className="h-5 w-5" />
          </span>
          管理后台
        </div>
        {[
          { key: "reviews", label: "审核队列" },
          { key: "auctions", label: "拍卖监控" },
          { key: "stats", label: "统计报表" },
          { key: "ops", label: "运维终端" },
        ].map((item) => (
          <button
            key={item.key}
            onClick={() => setPanel(item.key as Panel)}
            className={`mb-2 flex w-full items-center justify-between rounded-lg px-3 py-3 text-sm font-bold ${panel === item.key ? "bg-white text-slate-950" : "text-slate-300 hover:bg-slate-800"}`}
          >
            {item.label}
            <ChevronRight className="h-4 w-4" />
          </button>
        ))}
      </aside>

      <section className="p-4 sm:p-6 lg:p-8">
        <div className="mb-6 flex flex-col gap-4 lg:flex-row lg:items-end lg:justify-between">
          <div>
            <p className="text-sm font-bold text-indigo-600">ADMIN WORKBENCH</p>
            <h1 className="mt-1 text-3xl font-black text-slate-950">审核与运维大盘</h1>
          </div>
          <div className="rounded-lg bg-slate-950 px-4 py-3 font-mono text-xs text-emerald-300 shadow-float">
            <Command className="mr-2 inline h-4 w-4" />
            ops&gt; task_log scan OK · ws gateway online · retries 0
          </div>
        </div>

        <div className="mb-6 grid gap-4 md:grid-cols-4">
          <Kpi icon={<ClipboardCheck className="h-5 w-5" />} label="待审核" value={String(rows.length)} />
          <Kpi icon={<Gavel className="h-5 w-5" />} label="运行中拍卖" value={String(runningAuctions.length)} />
          <Kpi icon={<BarChart3 className="h-5 w-5" />} label="今日成交额" value={formatPrice(todayStats?.gmv_amount ?? 0)} />
          <Kpi icon={<ShieldAlert className="h-5 w-5" />} label="今日流拍" value={String(unsoldCount)} />
        </div>

        {panel === "reviews" ? (
          <div className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-card">
            <div className="border-b border-slate-200 px-5 py-4">
              <h2 className="text-base font-black text-slate-950">待审核拍品</h2>
            </div>
            {isLoading ? (
              <div className="p-5">
                <Skeleton className="h-44" />
              </div>
            ) : (
              <table className="w-full text-left text-sm">
                <thead className="bg-slate-50 text-xs uppercase text-slate-500">
                  <tr>
                    <th className="px-5 py-3">拍品</th>
                    <th className="px-5 py-3">卖家</th>
                    <th className="px-5 py-3">分类</th>
                    <th className="px-5 py-3">风险</th>
                    <th className="px-5 py-3">起拍价</th>
                  </tr>
                </thead>
                <tbody>
                  <AnimatePresence>
                    {rows.map((row) => (
                      <motion.tr layout key={row.id} exit={{ opacity: 0, height: 0 }} onClick={() => setSelected(row)} className="cursor-pointer border-t border-slate-100 hover:bg-indigo-50/40">
                        <td className="px-5 py-4 font-bold text-slate-950">{row.title}</td>
                        <td className="px-5 py-4 text-slate-600">{row.seller}</td>
                        <td className="px-5 py-4 text-slate-600">{row.category}</td>
                        <td className="px-5 py-4">
                          <span className={`rounded-full px-2.5 py-1 text-xs font-black ${riskClass(row.risk)}`}>{row.risk}</span>
                        </td>
                        <td className="tabular px-5 py-4 font-black text-slate-950">{formatPrice(row.price)}</td>
                      </motion.tr>
                    ))}
                  </AnimatePresence>
                </tbody>
              </table>
            )}
          </div>
        ) : null}

        {panel === "auctions" ? (
          <div className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-card">
            <div className="border-b border-slate-200 px-5 py-4">
              <h2 className="text-base font-black text-slate-950">拍卖监控</h2>
            </div>
            <table className="w-full text-left text-sm">
              <thead className="bg-slate-50 text-xs uppercase text-slate-500">
                <tr>
                  <th className="px-5 py-3">拍卖</th>
                  <th className="px-5 py-3">状态</th>
                  <th className="px-5 py-3">当前价</th>
                  <th className="px-5 py-3">最小加价</th>
                  <th className="px-5 py-3">结束时间</th>
                </tr>
              </thead>
              <tbody>
                {auctions.map((auction) => (
                  <tr key={auction.id} className="border-t border-slate-100">
                    <td className="px-5 py-4 font-bold text-slate-950">{auction.title}</td>
                    <td className="px-5 py-4 text-slate-600">{auction.status}</td>
                    <td className="tabular px-5 py-4 font-black text-slate-950">{formatPrice(auction.currentPrice)}</td>
                    <td className="tabular px-5 py-4 text-slate-600">{formatPrice(auction.minIncrement)}</td>
                    <td className="px-5 py-4 text-slate-600">{formatTime(auction.endTime)}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        ) : null}

        {panel === "stats" ? (
          <div className="grid gap-4 md:grid-cols-3">
            <Kpi icon={<Gavel className="h-5 w-5" />} label="今日成交" value={String(soldCount)} />
            <Kpi icon={<ShieldAlert className="h-5 w-5" />} label="今日流拍" value={String(unsoldCount)} />
            <Kpi icon={<BarChart3 className="h-5 w-5" />} label="今日 GMV" value={formatPrice(todayStats?.gmv_amount ?? 0)} />
          </div>
        ) : null}

        {panel === "ops" ? (
          <div className="grid gap-4 md:grid-cols-3">
            <Kpi icon={<Command className="h-5 w-5" />} label="通知重试队列" value="0" />
            <Kpi icon={<ClipboardCheck className="h-5 w-5" />} label="任务扫描" value="OK" />
            <Kpi icon={<ShieldAlert className="h-5 w-5" />} label="实时通道" value="ONLINE" />
          </div>
        ) : null}
      </section>

      <AnimatePresence>
        {selected ? (
          <motion.aside
            initial={{ x: 520, opacity: 0 }}
            animate={{ x: 0, opacity: 1 }}
            exit={{ x: 520, opacity: 0 }}
            transition={{ duration: 0.25 }}
            className="fixed bottom-0 right-0 top-0 z-50 w-full max-w-[500px] border-l border-white/60 bg-white/88 p-6 shadow-modal backdrop-blur"
          >
            <button onClick={() => setSelected(null)} className="absolute right-5 top-5 rounded-lg p-2 text-slate-500 hover:bg-slate-100">
              <X className="h-5 w-5" />
            </button>
            <p className="text-sm font-bold text-indigo-600">REVIEW DRAWER</p>
            <h2 className="mt-2 max-w-sm text-2xl font-black text-slate-950">{selected.title}</h2>
            <div className="mt-6 aspect-[4/3] rounded-lg border border-slate-200 bg-[linear-gradient(135deg,#eef2ff,#f8fafc)]" />
            <div className="mt-5 space-y-3 text-sm">
              <Info label="卖家" value={selected.seller} />
              <Info label="提交时间" value={formatTime(selected.submittedAt)} />
              <Info label="交易方式" value={selected.tradeMode || "校园当面交易"} />
              <Info label="交付地点" value={selected.location || "未填写"} />
            </div>
            <label className="mt-5 block text-sm font-bold text-slate-700" htmlFor="reject-reason">
              驳回原因
            </label>
            <textarea
              id="reject-reason"
              value={rejectReason}
              onChange={(event) => setRejectReason(event.target.value)}
              className="mt-2 min-h-20 w-full resize-none rounded-lg border border-slate-200 bg-white px-3 py-2 text-sm text-slate-700 outline-none transition focus:border-indigo-500 focus:ring-2 focus:ring-indigo-500/20"
            />
            {auditError ? (
              <div className="mt-4 flex items-center gap-2 rounded-lg border border-rose-200 bg-rose-50 px-3 py-2 text-sm text-rose-700">
                <AlertCircle className="h-4 w-4 shrink-0" />
                <span>{auditError}</span>
              </div>
            ) : null}
            <div className="mt-8 grid grid-cols-2 gap-3">
              <Button variant="danger" disabled={isAuditing} onClick={() => rejectMutation.mutate({ id: selected.id, reason: rejectReason })}>
                {rejectMutation.isPending ? <Loader2 className="h-4 w-4 animate-spin" /> : <X className="h-4 w-4" />}
                {rejectMutation.isPending ? "驳回中" : "驳回"}
              </Button>
              <Button disabled={isAuditing} onClick={() => approveMutation.mutate(selected.id)}>
                {approveMutation.isPending ? <Loader2 className="h-4 w-4 animate-spin" /> : <Check className="h-4 w-4" />}
                {approveMutation.isPending ? "批准中" : "批准"}
              </Button>
            </div>
          </motion.aside>
        ) : null}
      </AnimatePresence>
    </main>
  );
}

function Kpi({ icon, label, value }: { icon: ReactNode; label: string; value: string }) {
  return (
    <div className="rounded-lg border border-slate-200 bg-white p-5 shadow-card">
      <div className="mb-4 flex h-10 w-10 items-center justify-center rounded-lg bg-indigo-50 text-indigo-600">{icon}</div>
      <p className="text-sm font-bold text-slate-500">{label}</p>
      <p className="tabular mt-1 text-2xl font-black text-slate-950">{value}</p>
    </div>
  );
}

function Info({ label, value }: { label: string; value: string }) {
  return (
    <div className="rounded-lg border border-slate-200 bg-slate-50 px-4 py-3">
      <p className="text-xs font-bold text-slate-500">{label}</p>
      <p className="mt-1 font-bold text-slate-950">{value}</p>
    </div>
  );
}

function riskClass(risk: AdminReviewItem["risk"]) {
  if (risk === "HIGH") return "bg-rose-50 text-rose-700";
  if (risk === "MEDIUM") return "bg-amber-50 text-amber-700";
  return "bg-emerald-50 text-emerald-700";
}
