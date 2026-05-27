import { mockAuctions, mockBids, mockOrder, mockReviewItems } from "@/lib/api/mock-data";
import type { AdminReviewItem, AuctionItem, BidRecord, OrderSummary } from "@/types/auction";

type ApiMode = "mock" | "live";

const delay = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

function apiMode(): ApiMode {
  return process.env.NEXT_PUBLIC_API_MODE === "live" ? "live" : "mock";
}

async function liveFetch<T>(path: string, init?: RequestInit): Promise<T> {
  const baseUrl = process.env.NEXT_PUBLIC_API_BASE_URL ?? "http://127.0.0.1:18080";
  const response = await fetch(`${baseUrl}${path}`, init);
  if (!response.ok) {
    throw new Error(response.status === 404 ? "API route not ready" : `HTTP ${response.status}`);
  }
  return response.json() as Promise<T>;
}

export async function getAuctions(): Promise<AuctionItem[]> {
  if (apiMode() === "live") {
    return liveFetch<AuctionItem[]>("/api/auctions");
  }
  await delay(220);
  return mockAuctions;
}

export async function getAuction(id: string): Promise<AuctionItem> {
  if (apiMode() === "live") {
    return liveFetch<AuctionItem>(`/api/auctions/${id}`);
  }
  await delay(180);
  return mockAuctions.find((auction) => auction.id === id) ?? mockAuctions[0];
}

export async function getBids(auctionId: string): Promise<BidRecord[]> {
  if (apiMode() === "live") {
    return liveFetch<BidRecord[]>(`/api/auctions/${auctionId}/bids`);
  }
  await delay(180);
  return mockBids.filter((bid) => bid.auctionId === "AUC-1001" || bid.auctionId === auctionId);
}

export async function placeBid(auctionId: string, amount: number): Promise<BidRecord> {
  if (apiMode() === "live") {
    return liveFetch<BidRecord>(`/api/auctions/${auctionId}/bids`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ amount, idempotencyKey: crypto.randomUUID() })
    });
  }

  await delay(520);
  if (amount % 13 === 0) {
    throw new Error("409 Conflict");
  }
  if (amount % 17 === 0) {
    throw new Error("429 Too Many Requests");
  }

  return {
    id: `BID-${Date.now()}`,
    auctionId,
    bidder: "buyer_demo",
    amount,
    createdAt: new Date().toISOString(),
    status: "SUCCESS"
  };
}

export async function login(username: string) {
  if (apiMode() === "live") {
    return liveFetch<{ token: string; username: string }>("/api/auth/login", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, password: "mock" })
    });
  }
  await delay(480);
  return { token: "mock-token", username };
}

export async function getOrder(orderId: string): Promise<OrderSummary> {
  if (apiMode() === "live") {
    return liveFetch<OrderSummary>(`/api/orders/${orderId}`);
  }
  await delay(200);
  return { ...mockOrder, id: orderId };
}

export async function payOrder(orderId: string) {
  if (apiMode() === "live") {
    return liveFetch<{ status: string }>(`/api/orders/${orderId}/pay`, { method: "POST" });
  }
  await delay(900);
  return { status: "SUCCESS" };
}

export async function getAdminReviews(): Promise<AdminReviewItem[]> {
  if (apiMode() === "live") {
    return liveFetch<AdminReviewItem[]>("/api/admin/items/pending");
  }
  await delay(260);
  return mockReviewItems;
}
