export const queryKeys = {
  auctions: ["auctions"] as const,
  auction: (id: string) => ["auction", id] as const,
  bids: (auctionId: string) => ["bids", auctionId] as const,
  order: (orderId: string) => ["order", orderId] as const,
  adminReviews: ["admin-reviews"] as const
};
