import type { AuctionListQuery } from "@/types/auction";

export const queryKeys = {
  auctions: (query: AuctionListQuery = {}) => ["auctions", query] as const,
  auction: (id: string) => ["auction", id] as const,
  bids: (auctionId: string) => ["bids", auctionId] as const,
  order: (orderId: string) => ["order", orderId] as const,
  adminReviews: ["admin-reviews"] as const,
  adminAuctions: (status = "ALL") => ["admin-auctions", status] as const,
  adminDailyStatistics: (startDate: string, endDate: string) =>
    ["admin-daily-statistics", startDate, endDate] as const
};
