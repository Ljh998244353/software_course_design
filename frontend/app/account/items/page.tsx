"use client";

export const dynamic = "force-dynamic";

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { Archive, Loader2, PackageOpen } from "lucide-react";
import { AuthGuard } from "@/components/layout/auth-guard";
import { SiteNav } from "@/components/layout/site-nav";
import { Button } from "@/components/ui/button";
import { Skeleton } from "@/components/ui/skeleton";
import { Toast } from "@/components/ui/toast";
import { getMyItems, offlineItem } from "@/lib/api/client";
import { queryKeys } from "@/lib/query/keys";
import { formatPrice, formatTime } from "@/lib/utils/format";
import type { ItemStatus, MyItem } from "@/types/auction";

const statusLabels: Record<ItemStatus, string> = {
  DRAFT: "草稿",
  PENDING_AUDIT: "待审核",
  REJECTED: "已驳回",
  READY_FOR_AUCTION: "待建拍",
  IN_AUCTION: "拍卖中",
  SOLD: "已成交",
  UNSOLD: "已流拍",
  OFFLINE: "已下架",
};

const offlineAllowed = new Set<ItemStatus>(["DRAFT", "REJECTED", "READY_FOR_AUCTION", "UNSOLD"]);

export default function MyItemsPage() {
  return (
    <AuthGuard requireAuth>
      <MyItemsContent />
    </AuthGuard>
  );
}

function MyItemsContent() {
  const queryClient = useQueryClient();
  const { data = [], isLoading } = useQuery({
    queryKey: queryKeys.myItems("ALL"),
    queryFn: () => getMyItems(),
  });
  const mutation = useMutation({
    mutationFn: (itemId: string) => offlineItem(itemId),
    onSuccess: () => {
      void queryClient.invalidateQueries({ queryKey: queryKeys.myItems("ALL") });
    },
  });

  return (
    <>
      <SiteNav />
      <Toast message={mutation.isSuccess ? "拍品已下架" : mutation.error?.message ?? ""} tone={mutation.isError ? "danger" : "success"} />
      <main className="mx-auto max-w-6xl px-4 py-8 sm:px-6 lg:px-8">
        <div className="mb-6 flex items-end justify-between gap-4">
          <div>
            <p className="text-sm font-bold text-indigo-600">SELLER ITEMS</p>
            <h1 className="mt-1 text-3xl font-black text-slate-950">我的拍品</h1>
          </div>
          <Button variant="secondary" onClick={() => queryClient.invalidateQueries({ queryKey: queryKeys.myItems("ALL") })}>
            <PackageOpen className="h-4 w-4" />
            刷新
          </Button>
        </div>

        {isLoading ? (
          <Skeleton className="h-72" />
        ) : data.length === 0 ? (
          <div className="rounded-lg border border-slate-200 bg-white px-6 py-12 text-center shadow-card">
            <PackageOpen className="mx-auto mb-4 h-9 w-9 text-slate-400" />
            <p className="font-bold text-slate-700">暂无拍品</p>
          </div>
        ) : (
          <div className="grid gap-4">
            {data.map((item) => (
              <ItemRow
                key={item.itemId}
                item={item}
                pending={mutation.isPending}
                onOffline={() => mutation.mutate(String(item.itemId))}
              />
            ))}
          </div>
        )}
      </main>
    </>
  );
}

function ItemRow({ item, pending, onOffline }: { item: MyItem; pending: boolean; onOffline: () => void }) {
  const canOffline = offlineAllowed.has(item.status);
  return (
    <article className="grid gap-4 rounded-lg border border-slate-200 bg-white p-4 shadow-card md:grid-cols-[120px_1fr_auto] md:items-center">
      <img src={item.coverImageUrl} alt={item.title} className="h-24 w-full rounded-lg object-cover md:w-28" />
      <div>
        <div className="flex flex-wrap items-center gap-2">
          <h2 className="font-black text-slate-950">{item.title}</h2>
          <span className="rounded-full bg-slate-100 px-2.5 py-1 text-xs font-black text-slate-600">{statusLabels[item.status]}</span>
        </div>
        <div className="mt-2 flex flex-wrap gap-4 text-sm text-slate-500">
          <span className="tabular font-bold text-slate-800">{formatPrice(item.startPrice)}</span>
          <span>{formatTime(item.updatedAt)}</span>
        </div>
        {item.rejectReason ? <p className="mt-2 text-sm text-rose-600">{item.rejectReason}</p> : null}
      </div>
      <Button variant={canOffline ? "danger" : "secondary"} disabled={!canOffline || pending} onClick={onOffline}>
        {pending ? <Loader2 className="h-4 w-4 animate-spin" /> : <Archive className="h-4 w-4" />}
        下架
      </Button>
    </article>
  );
}
