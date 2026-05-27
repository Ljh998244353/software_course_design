"use client";

import { useParams } from "next/navigation";
import { useMutation, useQuery } from "@tanstack/react-query";
import { Check, CheckCircle2, CreditCard, Loader2, WalletCards } from "lucide-react";
import { useState } from "react";
import { Button } from "@/components/ui/button";
import { getOrder, payOrder } from "@/lib/api/client";
import { queryKeys } from "@/lib/query/keys";
import { formatPrice, formatTime } from "@/lib/utils/format";

export default function CheckoutPage() {
  const params = useParams<{ orderId: string }>();
  const [method, setMethod] = useState("campus");
  const orderQuery = useQuery({ queryKey: queryKeys.order(params.orderId), queryFn: () => getOrder(params.orderId) });
  const mutation = useMutation({ mutationFn: () => payOrder(params.orderId) });
  const order = orderQuery.data;

  return (
    <main className="flex min-h-screen items-center justify-center bg-slate-50 px-4 py-10">
      <div className="w-full max-w-xl rounded-lg border border-slate-200 bg-white p-6 shadow-modal">
        {mutation.isSuccess ? (
          <div className="py-14 text-center">
            <div className="mx-auto mb-5 flex h-16 w-16 items-center justify-center rounded-full bg-emerald-50 text-emerald-600">
              <CheckCircle2 className="h-9 w-9" />
            </div>
            <h1 className="text-3xl font-black text-slate-950">支付成功</h1>
            <p className="mt-3 text-sm leading-6 text-slate-600">订单已进入履约阶段，后端 live 模式将等待支付回调确认。</p>
          </div>
        ) : (
          <>
            <p className="text-sm font-bold text-indigo-600">SECURE CHECKOUT</p>
            <h1 className="mt-1 text-3xl font-black text-slate-950">订单支付</h1>
            <div className="mt-6 rounded-lg border border-slate-200 bg-slate-50 p-4">
              <p className="font-black text-slate-950">{order?.auctionTitle ?? "加载订单"}</p>
              <p className="mt-1 text-sm text-slate-500">支付截止：{order ? formatTime(order.deadline) : "-"}</p>
              <div className="mt-5 space-y-3 text-sm">
                <BillRow label="落锤价" value={order ? formatPrice(order.hammerPrice) : "-"} />
                <BillRow label="管理费" value={order ? formatPrice(order.serviceFee) : "-"} />
                <BillRow label="总计" value={order ? formatPrice(order.total) : "-"} strong />
              </div>
            </div>
            <div className="mt-6 grid gap-3 sm:grid-cols-2">
              {[
                { id: "campus", label: "校园一卡通", icon: CreditCard },
                { id: "wallet", label: "微信/支付宝模拟钱包", icon: WalletCards }
              ].map(({ id, label, icon: Icon }) => (
                <button key={id} onClick={() => setMethod(id)} className={`auction-transition relative rounded-lg border p-4 text-left ${method === id ? "border-indigo-500 bg-indigo-50" : "border-slate-200 bg-white hover:border-indigo-300"}`}>
                  <Icon className="mb-3 h-5 w-5 text-indigo-600" />
                  <p className="font-black text-slate-950">{label}</p>
                  {method === id ? <Check className="absolute right-4 top-4 h-5 w-5 rounded-full bg-emerald-500 p-0.5 text-white" /> : null}
                </button>
              ))}
            </div>
            <Button className="mt-6 w-full" disabled={!order || mutation.isPending} onClick={() => mutation.mutate()}>
              {mutation.isPending ? <Loader2 className="h-4 w-4 animate-spin" /> : null}
              {mutation.isPending ? "支付处理中" : "确认支付"}
            </Button>
          </>
        )}
      </div>
    </main>
  );
}

function BillRow({ label, value, strong = false }: { label: string; value: string; strong?: boolean }) {
  return (
    <div className={`flex items-center justify-between ${strong ? "border-t border-slate-200 pt-3 text-lg font-black text-slate-950" : "text-slate-600"}`}>
      <span>{label}</span>
      <span className="tabular">{value}</span>
    </div>
  );
}
