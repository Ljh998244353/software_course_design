import { mockAuctions, mockBids, mockOrder, mockReviewItems } from "@/lib/api/mock-data";
import type {
  AdminReviewItem,
  AuctionDetailRaw,
  AuctionItem,
  AuctionSummaryRaw,
  AuditItemResultRaw,
  BidHistoryEntryRaw,
  BidHistoryResponseRaw,
  BidRecord,
  CreateItemRaw,
  DailyStatisticsRaw,
  ItemImageRaw,
  LoginResponse,
  OrderDetailRaw,
  OrderSummary,
  PayOrderResultRaw,
  PaymentChannel,
  PendingAuditItemRaw,
  PlaceBidResultRaw,
  SubmitReviewRaw,
  UserProfile,
} from "@/types/auction";

type ApiMode = "mock" | "live";

const delay = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));
const fallbackAuctionImage =
  "https://images.unsplash.com/photo-1516321318423-f06f85e504b3?auto=format&fit=crop&w=900&q=80";

function apiBaseUrl(): string {
  return process.env.NEXT_PUBLIC_API_BASE_URL ?? "http://127.0.0.1:18080";
}

function normalizeAuctionImageUrl(imageUrl: string): string {
  const trimmed = imageUrl.trim();
  if (!trimmed) return fallbackAuctionImage;
  if (trimmed.startsWith("http://") || trimmed.startsWith("https://")) return trimmed;
  if (trimmed.startsWith("/")) return `${apiBaseUrl()}${trimmed}`;
  return trimmed;
}

// Fields not yet provided by the live API are marked with TODO.
function mapAuctionItem(raw: AuctionSummaryRaw): AuctionItem {
  return {
    id: String(raw.auction_id),
    title: raw.title,
    category: "", // TODO: not provided by live API
    imageUrl: normalizeAuctionImageUrl(raw.cover_image_url),
    currentPrice: raw.current_price,
    startPrice: 0, // TODO: not provided by list API
    minIncrement: 0, // TODO: not provided by list API
    endTime: raw.end_time,
    status: raw.status as AuctionItem["status"],
    highestBidder: "", // TODO: not provided by list API
    seller: { name: "", rating: 0, deals: 0 }, // TODO: not provided by live API
    watchers: 0, // TODO: not provided by live API
    location: "", // TODO: not provided by live API
    tags: [], // TODO: not provided by live API
    description: "", // TODO: not provided by live API
  };
}

function mapAuctionDetail(raw: AuctionDetailRaw): AuctionItem {
  return {
    ...mapAuctionItem(raw),
    startPrice: raw.start_price,
    minIncrement: raw.bid_step,
    highestBidder: raw.highest_bidder_masked,
  };
}

function mapBidHistoryEntry(raw: BidHistoryEntryRaw, auctionId: string): BidRecord {
  return {
    id: String(raw.bid_id),
    auctionId,
    bidder: raw.bidder_masked,
    amount: raw.bid_amount,
    createdAt: raw.bid_time,
    status: raw.bid_status === "WINNING" ? "SUCCESS" : "OUTBID",
  };
}

function mapPlaceBidResult(raw: PlaceBidResultRaw): BidRecord {
  return {
    id: String(raw.bid_id),
    auctionId: String(raw.auction_id),
    bidder: raw.highest_bidder_masked,
    amount: raw.bid_amount,
    createdAt: raw.server_time,
    status: raw.bid_status === "WINNING" ? "SUCCESS" : "OUTBID",
  };
}

function mapPendingAuditItem(raw: PendingAuditItemRaw): AdminReviewItem {
  return {
    id: String(raw.item_id),
    title: raw.title,
    seller: raw.seller_nickname || raw.seller_username,
    category: String(raw.category_id),
    risk: "LOW",
    submittedAt: raw.created_at,
    price: raw.start_price,
  };
}

function mapOrderDetail(raw: OrderDetailRaw): OrderSummary {
  const hammerPrice = raw.final_amount;
  return {
    id: String(raw.order_id),
    auctionTitle: raw.item_title,
    hammerPrice,
    serviceFee: 0,
    total: hammerPrice,
    seller: "",
    deadline: raw.pay_deadline_at,
  };
}

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
  const headers: Record<string, string> = {
    ...((init?.headers as Record<string, string>) ?? {}),
  };
  const token = getStoredToken();
  if (token && !headers["Authorization"]) {
    headers["Authorization"] = `Bearer ${token}`;
  }
  const response = await fetch(`${apiBaseUrl()}${path}`, { ...init, headers });
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
      json?.message
        ? `${response.status} ${json.message}`
        : (response.status === 404 ? "API route not ready" : `HTTP ${response.status}`)
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
    const raw = await liveFetch<AuctionSummaryRaw[]>("/api/auctions");
    return raw.map(mapAuctionItem);
  }
  await delay(220);
  return mockAuctions;
}

export async function getAuction(id: string): Promise<AuctionItem> {
  if (apiMode() === "live") {
    const raw = await liveFetch<AuctionDetailRaw>(`/api/auctions/${id}`);
    return mapAuctionDetail(raw);
  }
  await delay(180);
  return mockAuctions.find((auction) => auction.id === id) ?? mockAuctions[0];
}

export async function getBids(auctionId: string): Promise<BidRecord[]> {
  if (apiMode() === "live") {
    const raw = await liveFetch<BidHistoryResponseRaw>(`/api/auctions/${auctionId}/bids`);
    return raw.records.map((entry) => mapBidHistoryEntry(entry, auctionId));
  }
  await delay(180);
  return mockBids.filter((bid) => bid.auctionId === "AUC-1001" || bid.auctionId === auctionId);
}

export async function placeBid(auctionId: string, amount: number): Promise<BidRecord> {
  if (apiMode() === "live") {
    const raw = await liveFetch<PlaceBidResultRaw>(`/api/auctions/${auctionId}/bids`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ bid_amount: amount, request_id: crypto.randomUUID() }),
    });
    return mapPlaceBidResult(raw);
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
    const raw = await liveFetch<OrderDetailRaw>(`/api/orders/${orderId}`);
    return mapOrderDetail(raw);
  }
  await delay(200);
  return { ...mockOrder, id: orderId };
}

export async function payOrder(orderId: string, payChannel: PaymentChannel) {
  if (apiMode() === "live") {
    const raw = await liveFetch<PayOrderResultRaw>(`/api/orders/${orderId}/pay`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ pay_channel: payChannel }),
    });
    return { status: raw.pay_status };
  }
  await delay(900);
  return { status: "WAITING_CALLBACK" };
}

export async function getAdminReviews(): Promise<AdminReviewItem[]> {
  if (apiMode() === "live") {
    const raw = await liveFetch<PendingAuditItemRaw[]>("/api/admin/items/pending");
    return raw.map(mapPendingAuditItem);
  }
  await delay(260);
  return mockReviewItems;
}

export async function approveItem(itemId: string): Promise<AuditItemResultRaw> {
  if (apiMode() === "live") {
    return liveFetch<AuditItemResultRaw>(`/api/admin/items/${itemId}/approve`, {
      method: "POST",
    });
  }
  await delay(300);
  return { item_id: Number(itemId), old_status: "PENDING_AUDIT", new_status: "APPROVED", audit_result: "APPROVED", audited_at: new Date().toISOString() };
}

export async function rejectItem(itemId: string, reason?: string): Promise<AuditItemResultRaw> {
  if (apiMode() === "live") {
    return liveFetch<AuditItemResultRaw>(`/api/admin/items/${itemId}/reject`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ reason: reason ?? "" }),
    });
  }
  await delay(300);
  return { item_id: Number(itemId), old_status: "PENDING_AUDIT", new_status: "REJECTED", audit_result: "REJECTED", audited_at: new Date().toISOString() };
}

export async function getAdminDailyStatistics(startDate?: string, endDate?: string): Promise<DailyStatisticsRaw[]> {
  if (apiMode() === "live") {
    const params = new URLSearchParams();
    if (startDate) params.set("start_date", startDate);
    if (endDate) params.set("end_date", endDate);
    const qs = params.toString();
    return liveFetch<DailyStatisticsRaw[]>(`/api/admin/statistics/daily${qs ? `?${qs}` : ""}`);
  }
  await delay(200);
  return [{
    stat_id: 1,
    stat_date: startDate ?? new Date().toISOString().slice(0, 10),
    auction_count: 18,
    sold_count: 7,
    unsold_count: 2,
    bid_count: 126,
    gmv_amount: 18420,
    created_at: new Date().toISOString(),
  }];
}

export async function createItem(params: {
  title: string;
  description: string;
  category_id: number;
  start_price: number;
  cover_image_url: string;
}): Promise<CreateItemRaw> {
  if (apiMode() === "live") {
    return liveFetch<CreateItemRaw>("/api/items", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(params),
    });
  }
  await delay(400);
  return {
    item_id: Math.floor(Math.random() * 10000) + 100,
    item_status: "DRAFT",
    created_at: new Date().toISOString(),
  };
}

export async function addItemImage(
  itemId: string,
  params: {
    image_url: string;
    sort_no?: number;
    is_cover?: boolean;
  }
): Promise<ItemImageRaw> {
  if (apiMode() === "live") {
    return liveFetch<ItemImageRaw>(`/api/items/${itemId}/images`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(params),
    });
  }
  await delay(180);
  return {
    image_id: Math.floor(Math.random() * 10000) + 100,
    item_id: Number(itemId),
    image_url: params.image_url,
    sort_no: params.sort_no ?? 1,
    is_cover: params.is_cover ?? false,
    created_at: new Date().toISOString(),
  };
}

export async function submitItemForReview(itemId: string): Promise<SubmitReviewRaw> {
  if (apiMode() === "live") {
    return liveFetch<SubmitReviewRaw>(`/api/items/${itemId}/submit-review`, {
      method: "POST",
    });
  }
  await delay(300);
  return {
    item_id: Number(itemId),
    item_status: "PENDING_AUDIT",
    submitted_at: new Date().toISOString(),
  };
}
