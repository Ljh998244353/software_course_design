import type { AuctionListQuery } from "@/types/auction";

export const queryKeys = {
  auctions: (query: AuctionListQuery = {}) => ["auctions", query] as const,
  auction: (id: string) => ["auction", id] as const,
  bids: (auctionId: string) => ["bids", auctionId] as const,
  order: (orderId: string) => ["order", orderId] as const,
  myItems: (status = "ALL") => ["my-items", status] as const,
  myOrders: (role = "all", status = "ALL") => ["my-orders", role, status] as const,
  notifications: (unreadOnly = false) => ["notifications", unreadOnly] as const,
  orderReviews: (orderId: string) => ["order-reviews", orderId] as const,
  adminReviews: ["admin-reviews"] as const,
  adminAuctions: (status = "ALL") => ["admin-auctions", status] as const,
  adminDailyStatistics: (startDate: string, endDate: string) =>
    ["admin-daily-statistics", startDate, endDate] as const,
  adminOps: ["admin-ops"] as const
};
