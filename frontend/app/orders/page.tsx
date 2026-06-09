"use client";

export const dynamic = "force-dynamic";

import Link from "next/link";
import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { CheckCircle2, Loader2, PackageCheck, Send, Star } from "lucide-react";
import { useState } from "react";
import { AuthGuard } from "@/components/layout/auth-guard";
import { SiteNav } from "@/components/layout/site-nav";
import { Button } from "@/components/ui/button";
import { Skeleton } from "@/components/ui/skeleton";
import { Toast } from "@/components/ui/toast";
import { confirmReceipt, getMyOrders, shipOrder, submitOrderReview } from "@/lib/api/client";
import { queryKeys } from "@/lib/query/keys";
import { formatPrice, formatTime } from "@/lib/utils/format";
import type { OrderStatus, UserOrder } from "@/types/auction";

const orderStatusLabels: Record<OrderStatus, string> = {
  PENDING_PAYMENT: "待支付",
  PAID: "已支付",
  WAITING_DELIVERY: "待发货",
  SHIPPED: "已发货",
  COMPLETED: "已完成",
  REVIEWED: "已评价",
  CLOSED: "已关闭",
};

export default function OrdersPage() {
  return (
    <AuthGuard requireAuth>
      <OrdersContent />
    </AuthGuard>
  );
}

function OrdersContent() {
  const [role, setRole] = useState<"all" | "buyer" | "seller">("all");
  const queryClient = useQueryClient();
  const query = useQuery({
    queryKey: queryKeys.myOrders(role, "ALL"),
    queryFn: () => getMyOrders({ role: role === "all" ? undefined : role }),
  });
  const refresh = () => queryClient.invalidateQueries({ queryKey: queryKeys.myOrders(role, "ALL") });
  const shipMutation = useMutation({ mutationFn: (id: string) => shipOrder(id), onSuccess: refresh });
  const receiptMutation = useMutation({ mutationFn: (id: string) => confirmReceipt(id), onSuccess: refresh });
  const reviewMutation = useMutation({
    mutationFn: ({ id, rating, content }: { id: string; rating: number; content: string }) =>
      submitOrderReview(id, rating, content),
    onSuccess: refresh,
  });
  const toastMessage =
    shipMutation.error?.message ??
    receiptMutation.error?.message ??
    reviewMutation.error?.message ??
    (shipMutation.isSuccess ? "已确认发货" : receiptMutation.isSuccess ? "已确认收货" : reviewMutation.isSuccess ? "评价已提交" : "");
  const toastDanger = shipMutation.isError || receiptMutation.isError || reviewMutation.isError;

  return (
    <>
      <SiteNav />
      <Toast message={toastMessage} tone={toastDanger ? "danger" : "success"} />
      <main className="mx-auto max-w-6xl px-4 py-8 sm:px-6 lg:px-8">
        <div className="mb-6 flex flex-wrap items-end justify-between gap-4">
          <div>
            <p className="text-sm font-bold text-indigo-600">ORDER WORKFLOW</p>
            <h1 className="mt-1 text-3xl font-black text-slate-950">我的订单</h1>
          </div>
          <div className="flex rounded-lg border border-slate-200 bg-white p-1">
            {[
              { key: "all", label: "全部" },
              { key: "buyer", label: "买家" },
              { key: "seller", label: "卖家" },
            ].map((item) => (
              <button
                key={item.key}
                onClick={() => setRole(item.key as "all" | "buyer" | "seller")}
                className={`h-9 rounded-md px-3 text-sm font-bold ${role === item.key ? "bg-indigo-600 text-white" : "text-slate-600 hover:bg-slate-50"}`}
              >
                {item.label}
              </button>
            ))}
          </div>
        </div>

        {query.isLoading ? (
          <Skeleton className="h-80" />
        ) : (query.data ?? []).length === 0 ? (
          <div className="rounded-lg border border-slate-200 bg-white px-6 py-12 text-center shadow-card">
            <PackageCheck className="mx-auto mb-4 h-9 w-9 text-slate-400" />
            <p className="font-bold text-slate-700">暂无订单</p>
          </div>
        ) : (
          <div className="space-y-4">
            {(query.data ?? []).map((order) => (
              <OrderRow
                key={order.orderId}
                order={order}
                pending={shipMutation.isPending || receiptMutation.isPending || reviewMutation.isPending}
                onShip={() => shipMutation.mutate(String(order.orderId))}
                onReceipt={() => receiptMutation.mutate(String(order.orderId))}
                onReview={(rating, content) => reviewMutation.mutate({ id: String(order.orderId), rating, content })}
              />
            ))}
          </div>
        )}
      </main>
    </>
  );
}

function OrderRow({
  order,
  pending,
  onShip,
  onReceipt,
  onReview,
}: {
  order: UserOrder;
  pending: boolean;
  onShip: () => void;
  onReceipt: () => void;
  onReview: (rating: number, content: string) => void;
}) {
  const [rating, setRating] = useState(5);
  const [content, setContent] = useState("交易顺利，沟通及时");

  return (
    <article className="rounded-lg border border-slate-200 bg-white p-4 shadow-card">
      <div className="grid gap-4 md:grid-cols-[112px_1fr_auto] md:items-start">
        <img src={order.coverImageUrl || "https://images.unsplash.com/photo-1516321318423-f06f85e504b3?auto=format&fit=crop&w=900&q=80"} alt={order.itemTitle} className="h-24 w-full rounded-lg object-cover md:w-28" />
        <div>
          <div className="flex flex-wrap items-center gap-2">
            <h2 className="font-black text-slate-950">{order.itemTitle}</h2>
            <span className="rounded-full bg-slate-100 px-2.5 py-1 text-xs font-black text-slate-600">{orderStatusLabels[order.orderStatus]}</span>
          </div>
          <div className="mt-2 flex flex-wrap gap-4 text-sm text-slate-500">
            <span className="tabular font-black text-slate-950">{formatPrice(order.finalAmount)}</span>
            <span>买家 {order.buyerUsername}</span>
            <span>卖家 {order.sellerUsername}</span>
            <span>{formatTime(order.createdAt)}</span>
          </div>
        </div>
        <div className="flex flex-wrap justify-start gap-2 md:justify-end">
          {order.orderStatus === "PENDING_PAYMENT" ? (
            <Link className="auction-transition inline-flex min-h-10 items-center justify-center gap-2 rounded-lg bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-float hover:bg-indigo-700" href={`/checkout/${order.orderId}`}>
              支付
            </Link>
          ) : null}
          {order.orderStatus === "PAID" || order.orderStatus === "WAITING_DELIVERY" ? (
            <Button disabled={pending} onClick={onShip}>
              {pending ? <Loader2 className="h-4 w-4 animate-spin" /> : <Send className="h-4 w-4" />}
              发货
            </Button>
          ) : null}
          {order.orderStatus === "SHIPPED" ? (
            <Button disabled={pending} onClick={onReceipt}>
              {pending ? <Loader2 className="h-4 w-4 animate-spin" /> : <CheckCircle2 className="h-4 w-4" />}
              收货
            </Button>
          ) : null}
        </div>
      </div>
      {order.orderStatus === "COMPLETED" ? (
        <div className="mt-4 grid gap-3 border-t border-slate-100 pt-4 md:grid-cols-[120px_1fr_auto] md:items-center">
          <label className="text-sm font-bold text-slate-600">
            评分
            <select value={rating} onChange={(event) => setRating(Number(event.target.value))} className="mt-1 h-10 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500">
              {[5, 4, 3, 2, 1].map((value) => (
                <option key={value} value={value}>{value}</option>
              ))}
            </select>
          </label>
          <input value={content} onChange={(event) => setContent(event.target.value)} className="h-10 rounded-lg border border-slate-200 bg-slate-50 px-3 text-sm outline-none focus:border-indigo-500" />
          <Button disabled={pending} onClick={() => onReview(rating, content)}>
            {pending ? <Loader2 className="h-4 w-4 animate-spin" /> : <Star className="h-4 w-4" />}
            评价
          </Button>
        </div>
      ) : null}
    </article>
  );
}
