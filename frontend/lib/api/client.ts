import { mockAuctions, mockBids, mockOrder, mockReviewItems } from "@/lib/api/mock-data";
import type {
  AdminReviewItem,
  AuctionItem,
  BidRecord,
  LoginResponse,
  OrderSummary,
  UserProfile,
} from "@/types/auction";

type ApiMode = "mock" | "live";

const delay = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

function apiMode(): ApiMode {
  return process.env.NEXT_PUBLIC_API_MODE === "live" ? "live" : "mock";
}

function getStoredToken(): string | null {
  if (typeof window === "undefined") return null;
  return localStorage.getItem("auth_token");
}

function storeToken(token: string) {
  if (typeof window !== "undefined") {
    localStorage.setItem("auth_token", token);
  }
}

function clearToken() {
  if (typeof window !== "undefined") {
    localStorage.removeItem("auth_token");
  }
}

async function liveFetch<T>(path: string, init?: RequestInit): Promise<T> {
  const baseUrl = process.env.NEXT_PUBLIC_API_BASE_URL ?? "http://127.0.0.1:18080";
  const headers: Record<string, string> = {
    ...((init?.headers as Record<string, string>) ?? {}),
  };
  const token = getStoredToken();
  if (token && !headers["Authorization"]) {
    headers["Authorization"] = `Bearer ${token}`;
  }
  const response = await fetch(`${baseUrl}${path}`, { ...init, headers });
  const json = (await response.json().catch(() => null)) as {
    code?: number;
    message?: string;
    data?: T;
  } | null;
  if (!response.ok) {
    if (response.status === 401) {
      clearToken();
    }
    throw new Error(
      json?.message || (response.status === 404 ? "API route not ready" : `HTTP ${response.status}`)
    );
  }
  if (!json || typeof json.code !== "number") {
    throw new Error("invalid API response");
  }
  if (json.code !== 0) {
    throw new Error(json.message || `API error ${json.code}`);
  }
  return json.data as T;
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
      body: JSON.stringify({ amount, idempotencyKey: crypto.randomUUID() }),
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
    status: "SUCCESS",
  };
}

export async function login(username: string, password?: string): Promise<LoginResponse> {
  if (apiMode() === "live") {
    const result = await liveFetch<LoginResponse>("/api/auth/login", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, password: password ?? "" }),
    });
    storeToken(result.token);
    return result;
  }
  await delay(480);
  const result = {
    token: "mock-token",
    expire_at: new Date(Date.now() + 3600_000).toISOString(),
    user_info: {
      user_id: 1,
      username,
      nickname: username,
      role_code: "USER",
      status: "ACTIVE",
    },
  };
  storeToken(result.token);
  return result;
}

export async function getMe(): Promise<UserProfile> {
  if (apiMode() === "live") {
    return liveFetch<UserProfile>("/api/auth/me");
  }
  await delay(100);
  return {
    user_id: 1,
    username: "buyer_demo",
    nickname: "买家演示",
    role_code: "USER",
    status: "ACTIVE",
  };
}

export async function logout(): Promise<void> {
  if (apiMode() === "live") {
    await liveFetch<{ message: string }>("/api/auth/logout", { method: "POST" });
    clearToken();
    return;
  }
  await delay(100);
  clearToken();
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
