"use client";

import { useQuery } from "@tanstack/react-query";
import { AnimatePresence, motion } from "framer-motion";
import { BarChart3, Check, ChevronRight, ClipboardCheck, Command, Gavel, ShieldAlert, X } from "lucide-react";
import { useState } from "react";
import type { ReactNode } from "react";
import { Button } from "@/components/ui/button";
import { Skeleton } from "@/components/ui/skeleton";
import { getAdminReviews } from "@/lib/api/client";
import { queryKeys } from "@/lib/query/keys";
import { formatPrice, formatTime } from "@/lib/utils/format";
import type { AdminReviewItem } from "@/types/auction";

export default function AdminDashboardPage() {
  const [selected, setSelected] = useState<AdminReviewItem | null>(null);
  const [removed, setRemoved] = useState<string[]>([]);
  const { data = [], isLoading } = useQuery({ queryKey: queryKeys.adminReviews, queryFn: getAdminReviews });
  const rows = data.filter((row) => !removed.includes(row.id));

  function approve(id: string) {
    setRemoved((current) => [...current, id]);
    setSelected(null);
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
        {["审核队列", "拍卖监控", "统计报表", "运维终端"].map((item, index) => (
          <button key={item} className={`mb-2 flex w-full items-center justify-between rounded-lg px-3 py-3 text-sm font-bold ${index === 0 ? "bg-white text-slate-950" : "text-slate-300 hover:bg-slate-800"}`}>
            {item}
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
            ops&gt; task_log scan OK · notification retry queue 0
          </div>
        </div>

        <div className="mb-6 grid gap-4 md:grid-cols-4">
          <Kpi icon={<ClipboardCheck className="h-5 w-5" />} label="待审核" value={String(rows.length)} />
          <Kpi icon={<Gavel className="h-5 w-5" />} label="进行中拍卖" value="12" />
          <Kpi icon={<BarChart3 className="h-5 w-5" />} label="今日成交额" value="¥18,420" />
          <Kpi icon={<ShieldAlert className="h-5 w-5" />} label="风险事件" value="2" />
        </div>

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
                    <motion.tr
                      layout
                      key={row.id}
                      exit={{ opacity: 0, height: 0 }}
                      onClick={() => setSelected(row)}
                      className="cursor-pointer border-t border-slate-100 hover:bg-indigo-50/40"
                    >
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
              <Info label="OCR/关键词" value={selected.risk === "HIGH" ? "限量、未拆封、转卖风险" : "未发现明显违规词"} />
              <Info label="建议" value={selected.risk === "HIGH" ? "建议人工复核来源证明" : "可批准进入拍卖配置"} />
            </div>
            <div className="mt-8 grid grid-cols-2 gap-3">
              <Button variant="danger" onClick={() => approve(selected.id)}>
                <X className="h-4 w-4" />
                驳回
              </Button>
              <Button onClick={() => approve(selected.id)}>
                <Check className="h-4 w-4" />
                批准
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
  if (risk === "HIGH") {
    return "bg-rose-50 text-rose-700";
  }
  if (risk === "MEDIUM") {
    return "bg-amber-50 text-amber-700";
  }
  return "bg-emerald-50 text-emerald-700";
}
