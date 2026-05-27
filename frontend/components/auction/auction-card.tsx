import Image from "next/image";
import Link from "next/link";
import { ArrowUpRight, Clock3, Eye } from "lucide-react";
import type { AuctionItem } from "@/types/auction";
import { countdownLabel, countdownTone, formatPrice } from "@/lib/utils/format";

export function AuctionCard({ auction }: { auction: AuctionItem }) {
  const urgent = countdownTone(auction.endTime).includes("rose");

  return (
    <Link
      href={`/auction/detail/${auction.id}`}
      className="auction-transition group overflow-hidden rounded-lg border border-slate-200 bg-white shadow-card hover:-translate-y-1 hover:border-indigo-300 hover:shadow-float"
    >
      <div className="relative aspect-[4/3] overflow-hidden bg-slate-100">
        <Image src={auction.imageUrl} alt={auction.title} fill className="object-cover transition duration-500 group-hover:scale-105" sizes="(min-width: 1024px) 25vw, 100vw" />
        <div className="absolute left-3 top-3 rounded-full bg-white/90 px-3 py-1 text-xs font-semibold text-slate-700 shadow-card">
          {auction.category}
        </div>
      </div>
      <div className="space-y-4 p-4">
        <div>
          <h3 className="line-clamp-1 text-base font-bold text-slate-950">{auction.title}</h3>
          <p className="mt-1 line-clamp-2 text-sm leading-6 text-slate-600">{auction.description}</p>
        </div>
        <div className="flex items-end justify-between gap-3">
          <div>
            <p className="text-xs font-medium text-slate-400">当前最高价</p>
            <p className="tabular text-2xl font-black text-slate-950">{formatPrice(auction.currentPrice)}</p>
          </div>
          <span className={`tabular flex items-center gap-1 rounded-full border px-2.5 py-1 text-xs font-bold ${countdownTone(auction.endTime)} ${urgent ? "urgent-pulse" : ""}`}>
            <Clock3 className="h-3.5 w-3.5" />
            {countdownLabel(auction.endTime)}
          </span>
        </div>
        <div className="flex items-center justify-between border-t border-slate-100 pt-3 text-xs font-medium text-slate-500">
          <span className="flex items-center gap-1">
            <Eye className="h-3.5 w-3.5" />
            {auction.watchers} 人关注
          </span>
          <span className="flex translate-x-2 items-center gap-1 text-indigo-600 opacity-0 transition group-hover:translate-x-0 group-hover:opacity-100">
            查看竞价
            <ArrowUpRight className="h-4 w-4" />
          </span>
        </div>
      </div>
    </Link>
  );
}
