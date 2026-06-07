import type {
  AdminReviewItem,
  AdminAuctionListRaw,
  AdminAuctionSummaryRaw,
  AuctionDetailRaw,
  AuctionItem,
  AuctionListQuery,
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
const fallbackAuctionImage =
  "https://images.unsplash.com/photo-1516321318423-f06f85e504b3?auto=format&fit=crop&w=900&q=80";

type ApiEnvelope<T> = {
  code?: number;
  message?: string;
  data?: T;
};

export class ApiError extends Error {
  readonly status: number;
  readonly code?: number;
  readonly detail?: string;

  constructor(params: { message: string; status: number; code?: number; detail?: string }) {
    super(params.message);
    this.name = "ApiError";
    this.status = params.status;
    this.code = params.code;
    this.detail = params.detail;
  }
}

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

function mapAuctionItem(raw: AuctionSummaryRaw): AuctionItem {
  const tags = (() => {
    try {
      const parsed = JSON.parse(raw.tags_json || "[]");
      return Array.isArray(parsed) ? parsed.filter((tag): tag is string => typeof tag === "string") : [];
    } catch {
      return [];
    }
  })();
  return {
    id: String(raw.auction_id),
    title: raw.title,
    category: raw.category_name,
    imageUrl: normalizeAuctionImageUrl(raw.cover_image_url),
    currentPrice: raw.current_price,
    startPrice: raw.start_price,
    minIncrement: raw.bid_step,
    endTime: raw.end_time,
    status: raw.status as AuctionItem["status"],
    highestBidder: "",
    seller: { name: raw.seller_username, rating: raw.seller_rating, deals: raw.seller_deals },
    watchers: raw.watcher_count,
    location: raw.location,
    tags,
    description: raw.description,
    tradeMode: raw.trade_mode,
    sellerUsername: raw.seller_username,
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
    location: "",
    tradeMode: "",
  };
}

function mapAdminAuctionSummary(raw: AdminAuctionSummaryRaw): AuctionItem {
  return {
    id: String(raw.auctionId),
    title: raw.title,
    category: "拍卖",
    imageUrl: normalizeAuctionImageUrl(raw.coverImageUrl),
    currentPrice: raw.currentPrice,
    startPrice: raw.startPrice,
    minIncrement: raw.bidStep,
    endTime: raw.endTime,
    status: raw.status as AuctionItem["status"],
    highestBidder: raw.highestBidderId ? String(raw.highestBidderId) : "",
    seller: { name: `卖家#${raw.sellerId}`, rating: 0, deals: 0 },
    watchers: 0,
    location: "",
    tags: [],
    description: "",
    tradeMode: "",
    sellerUsername: `seller_${raw.sellerId}`,
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

  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), 8000);

  try {
    const response = await fetch(`${apiBaseUrl()}${path}`, {
      ...init,
      headers,
      signal: controller.signal,
    });
    clearTimeout(timeoutId);

    const json = (await response.json().catch(() => null)) as ApiEnvelope<T> | null;
    if (!response.ok) {
      if (response.status === 401) {
        clearToken();
      }
      throw new ApiError({
        status: response.status,
        code: json?.code,
        detail: json?.message,
        message:
          json?.message ??
          (response.status === 404 ? "API route not ready" : `HTTP ${response.status}`),
      });
    }
    if (!json || typeof json.code !== "number") {
      throw new ApiError({
        status: response.status,
        message: "invalid API response",
      });
    }
    if (json.code !== 0) {
      throw new ApiError({
        status: response.status,
        code: json.code,
        detail: json.message,
        message: json.message || `API error ${json.code}`,
      });
    }
    return json.data as T;
  } catch (error) {
    clearTimeout(timeoutId);
    if (error instanceof DOMException && error.name === "AbortError") {
      throw new ApiError({
        status: 0,
        message: "请求超时，请检查后端服务是否运行",
      });
    }
    if (error instanceof TypeError) {
      throw new ApiError({
        status: 0,
        message: "无法连接后端服务，请确认服务已启动且地址可访问",
      });
    }
    throw error;
  }
}

export function describeLoginError(error: unknown): string {
  if (error instanceof ApiError) {
    if (error.status === 0) {
      return error.message;
    }
    switch (error.code) {
      case 4001:
        if (error.detail?.includes("principal")) return "用户名不能为空";
        if (error.detail?.includes("password")) return "密码不能为空";
        return "登录请求参数不完整";
      case 4102:
        return "用户名不存在或密码错误";
      case 4106:
        return "账号已被冻结，请联系管理员";
      case 4107:
        return "账号已被禁用，请联系管理员";
      case 5004:
      case 5005:
        return "数据库暂不可用，请稍后重试";
      case 5001:
        return "服务内部错误，请查看后端日志";
      default:
        if (error.status === 404) return "登录接口不存在，请确认后端版本和路由注册";
        if (error.status === 401) return error.detail || "认证失败";
        if (error.status >= 500) return "后端服务异常，请稍后重试";
        return error.detail || error.message;
    }
  }
  if (error instanceof Error) {
    return error.message;
  }
  return "登录失败";
}

export async function getAuctions(query: AuctionListQuery = {}): Promise<AuctionItem[]> {
  const params = new URLSearchParams();
  if (query.keyword) params.set("keyword", query.keyword);
  if (query.status) params.set("status", query.status);
  if (query.category) params.set("category", query.category);
  if (typeof query.priceMin === "number") params.set("price_min", String(query.priceMin));
  if (typeof query.priceMax === "number") params.set("price_max", String(query.priceMax));
  if (typeof query.sellerRating === "number") params.set("seller_rating", String(query.sellerRating));
  if (typeof query.sellerHasDeals === "boolean") params.set("seller_has_deals", query.sellerHasDeals ? "true" : "false");
  if (query.tradeMode) params.set("trade_mode", query.tradeMode);
  if (query.pageNo) params.set("page_no", String(query.pageNo));
  if (query.pageSize) params.set("page_size", String(query.pageSize));
  const raw = await liveFetch<AuctionSummaryRaw[]>(`/api/auctions${params.toString() ? `?${params.toString()}` : ""}`);
  return raw.map(mapAuctionItem);
}

export async function getAuction(id: string): Promise<AuctionItem> {
  const raw = await liveFetch<AuctionDetailRaw>(`/api/auctions/${id}`);
  return mapAuctionDetail(raw);
}

export async function getBids(auctionId: string): Promise<BidRecord[]> {
  const raw = await liveFetch<BidHistoryResponseRaw>(`/api/auctions/${auctionId}/bids`);
  return raw.records.map((entry) => mapBidHistoryEntry(entry, auctionId));
}

export async function placeBid(auctionId: string, amount: number): Promise<BidRecord> {
  const raw = await liveFetch<PlaceBidResultRaw>(`/api/auctions/${auctionId}/bids`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ bid_amount: amount, request_id: crypto.randomUUID() }),
  });
  return mapPlaceBidResult(raw);
}

export async function login(username: string, password?: string): Promise<LoginResponse> {
  const result = await liveFetch<LoginResponse>("/api/auth/login", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ username, password: password ?? "" }),
  });
  storeToken(result.token);
  return result;
}

export async function getMe(): Promise<UserProfile> {
  return liveFetch<UserProfile>("/api/auth/me");
}

export async function logout(): Promise<void> {
  await liveFetch<{ message: string }>("/api/auth/logout", { method: "POST" });
  clearToken();
}

export async function getOrder(orderId: string): Promise<OrderSummary> {
  const raw = await liveFetch<OrderDetailRaw>(`/api/orders/${orderId}`);
  return mapOrderDetail(raw);
}

export async function payOrder(orderId: string, payChannel: PaymentChannel) {
  const raw = await liveFetch<PayOrderResultRaw>(`/api/orders/${orderId}/pay`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ pay_channel: payChannel }),
  });
  return { status: raw.pay_status };
}

export async function getAdminReviews(): Promise<AdminReviewItem[]> {
  const raw = await liveFetch<PendingAuditItemRaw[]>("/api/admin/items/pending");
  return raw.map(mapPendingAuditItem);
}

export async function getAdminAuctions(status?: string): Promise<AuctionItem[]> {
  const query = status && status !== "ALL" ? `?status=${encodeURIComponent(status)}` : "";
  const raw = await liveFetch<AdminAuctionListRaw>(`/api/admin/auctions${query}`);
  return raw.list.map(mapAdminAuctionSummary);
}

export async function approveItem(itemId: string): Promise<AuditItemResultRaw> {
  return liveFetch<AuditItemResultRaw>(`/api/admin/items/${itemId}/approve`, {
    method: "POST",
  });
}

export async function rejectItem(itemId: string, reason?: string): Promise<AuditItemResultRaw> {
  return liveFetch<AuditItemResultRaw>(`/api/admin/items/${itemId}/reject`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ reason: reason ?? "" }),
  });
}

export async function getAdminDailyStatistics(startDate?: string, endDate?: string): Promise<DailyStatisticsRaw[]> {
  const params = new URLSearchParams();
  if (startDate) params.set("start_date", startDate);
  if (endDate) params.set("end_date", endDate);
  const qs = params.toString();
  return liveFetch<DailyStatisticsRaw[]>(`/api/admin/statistics/daily${qs ? `?${qs}` : ""}`);
}

export async function createItem(params: {
  title: string;
  description: string;
  category_id: number;
  start_price: number;
  trade_mode: string;
  location: string;
  tags_json: string[] | string;
  suggested_bid_step: number;
  suggested_anti_sniping_window_seconds: number;
  suggested_extend_seconds: number;
  cover_image_url: string;
  request_id?: string;
}): Promise<CreateItemRaw> {
  return liveFetch<CreateItemRaw>("/api/items", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(params),
  });
}

export async function addItemImage(
  itemId: string,
  params: {
    image_url: string;
    sort_no?: number;
    is_cover?: boolean;
  }
): Promise<ItemImageRaw> {
  return liveFetch<ItemImageRaw>(`/api/items/${itemId}/images`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(params),
  });
}

export async function submitItemForReview(itemId: string): Promise<SubmitReviewRaw> {
  return liveFetch<SubmitReviewRaw>(`/api/items/${itemId}/submit-review`, {
    method: "POST",
  });
}
