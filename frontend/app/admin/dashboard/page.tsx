"use client";

export const dynamic = "force-dynamic";

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { AnimatePresence, motion } from "framer-motion";
import { AlertCircle, BarChart3, Check, ChevronRight, ClipboardCheck, Command, Gavel, Loader2, LogOut, PlusCircle, ShieldAlert, X } from "lucide-react";
import { useRouter } from "next/navigation";
import { useEffect, useMemo, useState } from "react";
import type { ReactNode } from "react";
import { AuthGuard } from "@/components/layout/auth-guard";
import { Button } from "@/components/ui/button";
import { Skeleton } from "@/components/ui/skeleton";
import { approveItem, createAdminAuction, getAdminAuctions, getAdminDailyStatistics, getAdminReviews, rejectItem } from "@/lib/api/client";
import { queryKeys } from "@/lib/query/keys";
import { formatPrice, formatTime } from "@/lib/utils/format";
import type { AdminReviewDecision, AdminReviewItem, AuctionItem } from "@/types/auction";

type Panel = "reviews" | "auctions" | "stats" | "ops";
const recentDecisionStorageKey = "admin-recent-review-decisions";

export default function AdminDashboardPage() {
  return (
    <AuthGuard requireAuth requireAdmin>
      <AdminDashboardContent />
    </AuthGuard>
  );
}

function AdminDashboardContent() {
  const router = useRouter();
  const queryClient = useQueryClient();
  const [panel, setPanel] = useState<Panel>("reviews");
  const [selected, setSelected] = useState<AdminReviewItem | null>(null);
  const [removed, setRemoved] = useState<string[]>([]);
  const [recentDecisions, setRecentDecisions] = useState<AdminReviewDecision[]>([]);
  const [rejectReason, setRejectReason] = useState("不符合平台审核要求");
  const [auctionFormOpenFor, setAuctionFormOpenFor] = useState<string | null>(null);
  const [auctionStartTime, setAuctionStartTime] = useState(defaultAuctionStartTime);
  const [auctionEndTime, setAuctionEndTime] = useState(defaultAuctionEndTime);
  const [auctionStartPrice, setAuctionStartPrice] = useState("0");
  const [auctionBidStep, setAuctionBidStep] = useState("20");
  const [auctionAntiSnipingWindow, setAuctionAntiSnipingWindow] = useState("300");
  const [auctionExtendSeconds, setAuctionExtendSeconds] = useState("180");
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
    onSuccess: (_result, id) => finishAudit(id, "APPROVED"),
  });
  const rejectMutation = useMutation({
    mutationFn: ({ id, reason }: { id: string; reason: string }) => rejectItem(id, reason),
    onSuccess: (_result, variables) => finishAudit(variables.id, "REJECTED", variables.reason),
  });
  const createAuctionMutation = useMutation({
    mutationFn: createAdminAuction,
    onSuccess: (result, variables) => {
      setRecentDecisions((current) =>
        current.map((entry) =>
          entry.itemId === String(variables.itemId) && entry.result === "APPROVED"
            ? {
                ...entry,
                auctionId: String(result.auctionId),
                auctionStatus: result.status,
              }
            : entry,
        ),
      );
      setAuctionFormOpenFor(null);
      void queryClient.invalidateQueries({ queryKey: queryKeys.adminAuctions("ALL") });
    },
  });
  const auditError = approveMutation.error?.message ?? rejectMutation.error?.message;
  const isAuditing = approveMutation.isPending || rejectMutation.isPending;
  const createAuctionError = createAuctionMutation.error?.message;
  const soldCount = todayStats?.sold_count ?? 0;
  const unsoldCount = todayStats?.unsold_count ?? 0;
  const runningAuctions = useMemo(() => auctions.filter((auction) => auction.status === "RUNNING"), [auctions]);

  useEffect(() => {
    if (typeof window === "undefined") return;
    const saved = window.localStorage.getItem(recentDecisionStorageKey);
    if (!saved) return;
    try {
      const parsed = JSON.parse(saved) as AdminReviewDecision[];
      if (Array.isArray(parsed)) {
        setRecentDecisions(parsed);
      }
    } catch {
      window.localStorage.removeItem(recentDecisionStorageKey);
    }
  }, []);

  useEffect(() => {
    if (typeof window === "undefined") return;
    window.localStorage.setItem(recentDecisionStorageKey, JSON.stringify(recentDecisions));
  }, [recentDecisions]);

  function handleLeaveAdmin() {
    router.replace("/auction/hall");
  }

  function finishAudit(id: string, result: "APPROVED" | "REJECTED", comment?: string) {
    const processed = data.find((row) => row.id === id) ?? selected;
    if (processed) {
      setRecentDecisions((current) => [
        {
          itemId: processed.id,
          title: processed.title,
          seller: processed.seller,
          price: processed.price,
          result,
          auditedAt: new Date().toISOString(),
          comment,
          auctionId: null,
          auctionStatus: null,
        },
        ...current,
      ].slice(0, 8));
    }
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
          <div className="flex flex-wrap items-center gap-3">
            <div className="rounded-lg bg-slate-950 px-4 py-3 font-mono text-xs text-emerald-300 shadow-float">
              <Command className="mr-2 inline h-4 w-4" />
              ops&gt; task_log scan OK · ws gateway online · retries 0
            </div>
            <Button variant="secondary" onClick={handleLeaveAdmin}>
              <LogOut className="h-4 w-4" />
              退出后台
            </Button>
          </div>
        </div>

        <div className="mb-6 grid gap-4 md:grid-cols-4">
          <Kpi icon={<ClipboardCheck className="h-5 w-5" />} label="待审核" value={String(rows.length)} />
          <Kpi icon={<Gavel className="h-5 w-5" />} label="运行中拍卖" value={String(runningAuctions.length)} />
          <Kpi icon={<BarChart3 className="h-5 w-5" />} label="今日成交额" value={formatPrice(todayStats?.gmv_amount ?? 0)} />
          <Kpi icon={<ShieldAlert className="h-5 w-5" />} label="今日流拍" value={String(unsoldCount)} />
        </div>

        {panel === "reviews" ? (
          <div className="space-y-6">
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

            <div className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-card">
              <div className="border-b border-slate-200 px-5 py-4">
                <h2 className="text-base font-black text-slate-950">最近处理记录</h2>
              </div>
              {recentDecisions.length === 0 ? (
                <div className="px-5 py-8 text-sm text-slate-500">当前还没有本次会话内的审核处理记录。</div>
              ) : (
                <table className="w-full text-left text-sm">
                  <thead className="bg-slate-50 text-xs uppercase text-slate-500">
                    <tr>
                      <th className="px-5 py-3">拍品</th>
                      <th className="px-5 py-3">卖家</th>
                      <th className="px-5 py-3">结果</th>
                      <th className="px-5 py-3">处理时间</th>
                    </tr>
                  </thead>
                  <tbody>
                    {recentDecisions.map((decision) => (
                      <tr key={`${decision.itemId}-${decision.auditedAt}`} className="border-t border-slate-100">
                        <td className="px-5 py-4 font-bold text-slate-950">
                          <div>{decision.title}</div>
                          <div className="mt-1 text-xs font-medium text-slate-400">#{decision.itemId}</div>
                        </td>
                        <td className="px-5 py-4 text-slate-600">{decision.seller}</td>
                        <td className="px-5 py-4">
                          <span className={`rounded-full px-2.5 py-1 text-xs font-black ${decision.result === "APPROVED" ? "bg-emerald-50 text-emerald-700" : "bg-rose-50 text-rose-700"}`}>
                            {decision.result === "APPROVED" ? "已批准" : "已驳回"}
                          </span>
                          {decision.comment ? <div className="mt-2 text-xs text-slate-500">{decision.comment}</div> : null}
                          {decision.result === "APPROVED" ? (
                            <div className="mt-3">
                              {decision.auctionId ? (
                                <div className="text-xs font-medium text-emerald-700">
                                  已创建拍卖 #{decision.auctionId}
                                  {decision.auctionStatus ? ` · ${decision.auctionStatus}` : ""}
                                </div>
                              ) : (
                                <div className="space-y-3 rounded-lg border border-slate-200 bg-slate-50 p-3">
                                  <div className="flex items-center justify-between gap-3">
                                    <div className="text-xs font-medium text-slate-500">审核通过后还需要管理员创建正式拍卖。</div>
                                    <Button
                                      onClick={() => {
                                        setAuctionFormOpenFor((current) => (current === decision.itemId ? null : decision.itemId));
                                        setAuctionStartPrice(String(decision.price));
                                      }}
                                    >
                                      <PlusCircle className="h-4 w-4" />
                                      创建拍卖
                                    </Button>
                                  </div>
                                  {auctionFormOpenFor === decision.itemId ? (
                                    <div className="grid gap-3 md:grid-cols-2">
                                      <label className="text-xs font-bold text-slate-600">
                                        开始时间
                                        <input
                                          type="datetime-local"
                                          value={auctionStartTime}
                                          onChange={(event) => setAuctionStartTime(event.target.value)}
                                          className="mt-1 h-10 w-full rounded-lg border border-slate-200 bg-white px-3 text-sm outline-none focus:border-indigo-500"
                                        />
                                      </label>
                                      <label className="text-xs font-bold text-slate-600">
                                        结束时间
                                        <input
                                          type="datetime-local"
                                          value={auctionEndTime}
                                          onChange={(event) => setAuctionEndTime(event.target.value)}
                                          className="mt-1 h-10 w-full rounded-lg border border-slate-200 bg-white px-3 text-sm outline-none focus:border-indigo-500"
                                        />
                                      </label>
                                      <label className="text-xs font-bold text-slate-600">
                                        起拍价
                                        <input
                                          value={auctionStartPrice}
                                          onChange={(event) => setAuctionStartPrice(event.target.value)}
                                          className="mt-1 h-10 w-full rounded-lg border border-slate-200 bg-white px-3 text-sm outline-none focus:border-indigo-500"
                                        />
                                      </label>
                                      <label className="text-xs font-bold text-slate-600">
                                        最小加价
                                        <input
                                          value={auctionBidStep}
                                          onChange={(event) => setAuctionBidStep(event.target.value)}
                                          className="mt-1 h-10 w-full rounded-lg border border-slate-200 bg-white px-3 text-sm outline-none focus:border-indigo-500"
                                        />
                                      </label>
                                      <label className="text-xs font-bold text-slate-600">
                                        延时窗口秒数
                                        <input
                                          value={auctionAntiSnipingWindow}
                                          onChange={(event) => setAuctionAntiSnipingWindow(event.target.value)}
                                          className="mt-1 h-10 w-full rounded-lg border border-slate-200 bg-white px-3 text-sm outline-none focus:border-indigo-500"
                                        />
                                      </label>
                                      <label className="text-xs font-bold text-slate-600">
                                        每次延长秒数
                                        <input
                                          value={auctionExtendSeconds}
                                          onChange={(event) => setAuctionExtendSeconds(event.target.value)}
                                          className="mt-1 h-10 w-full rounded-lg border border-slate-200 bg-white px-3 text-sm outline-none focus:border-indigo-500"
                                        />
                                      </label>
                                      <div className="md:col-span-2">
                                        {createAuctionError ? <div className="mb-2 text-xs text-rose-600">{createAuctionError}</div> : null}
                                        <Button
                                          disabled={createAuctionMutation.isPending}
                                          onClick={() =>
                                            createAuctionMutation.mutate({
                                              itemId: Number(decision.itemId),
                                              startTime: normalizeDateTimeLocalToIso(auctionStartTime),
                                              endTime: normalizeDateTimeLocalToIso(auctionEndTime),
                                              startPrice: Number(auctionStartPrice),
                                              bidStep: Number(auctionBidStep),
                                              antiSnipingWindowSeconds: Number(auctionAntiSnipingWindow),
                                              extendSeconds: Number(auctionExtendSeconds),
                                            })
                                          }
                                        >
                                          {createAuctionMutation.isPending ? <Loader2 className="h-4 w-4 animate-spin" /> : <PlusCircle className="h-4 w-4" />}
                                          {createAuctionMutation.isPending ? "创建中" : "确认创建拍卖"}
                                        </Button>
                                      </div>
                                    </div>
                                  ) : null}
                                </div>
                              )}
                            </div>
                          ) : null}
                        </td>
                        <td className="px-5 py-4 text-slate-600">{formatTime(decision.auditedAt)}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              )}
            </div>
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
            className="fixed bottom-0 right-0 top-0 z-50 flex w-full max-w-[500px] flex-col overflow-hidden border-l border-white/60 bg-white/88 shadow-modal backdrop-blur"
          >
            <div className="flex items-start justify-between gap-4 border-b border-slate-200/80 px-6 pb-4 pt-6">
              <div className="min-w-0">
                <p className="text-sm font-bold text-indigo-600">REVIEW DRAWER</p>
                <h2 className="mt-2 break-words text-2xl font-black text-slate-950">{selected.title}</h2>
              </div>
              <button onClick={() => setSelected(null)} className="shrink-0 rounded-lg p-2 text-slate-500 hover:bg-slate-100">
                <X className="h-5 w-5" />
              </button>
            </div>

            <div className="min-h-0 flex-1 overflow-y-auto px-6 py-5">
              {selected.imageUrl ? (
                <div className="overflow-hidden rounded-lg border border-slate-200 bg-slate-50">
                  <img
                    src={selected.imageUrl}
                    alt={selected.title}
                    className="aspect-[4/3] w-full object-cover"
                  />
                </div>
              ) : (
                <div className="aspect-[4/3] rounded-lg border border-slate-200 bg-[linear-gradient(135deg,#eef2ff,#f8fafc)]" />
              )}
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
            </div>

            <div className="border-t border-slate-200/80 bg-white/80 px-6 py-4 backdrop-blur">
              <div className="grid grid-cols-2 gap-3">
                <Button variant="danger" disabled={isAuditing} onClick={() => rejectMutation.mutate({ id: selected.id, reason: rejectReason })}>
                  {rejectMutation.isPending ? <Loader2 className="h-4 w-4 animate-spin" /> : <X className="h-4 w-4" />}
                  {rejectMutation.isPending ? "驳回中" : "驳回"}
                </Button>
                <Button disabled={isAuditing} onClick={() => approveMutation.mutate(selected.id)}>
                  {approveMutation.isPending ? <Loader2 className="h-4 w-4 animate-spin" /> : <Check className="h-4 w-4" />}
                  {approveMutation.isPending ? "批准中" : "批准"}
                </Button>
              </div>
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

function defaultAuctionStartTime() {
  const next = new Date(Date.now() + 60 * 1000);
  return toDatetimeLocalValue(next);
}

function defaultAuctionEndTime() {
  const next = new Date(Date.now() + 24 * 60 * 60 * 1000);
  return toDatetimeLocalValue(next);
}

function toDatetimeLocalValue(date: Date) {
  const offsetMs = date.getTimezoneOffset() * 60 * 1000;
  return new Date(date.getTime() - offsetMs).toISOString().slice(0, 16);
}

function normalizeDateTimeLocalToIso(value: string) {
  return `${value.replace("T", " ")}:00`;
}
