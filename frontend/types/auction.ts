export type AuctionStatus = "PENDING_START" | "RUNNING" | "SETTLING" | "SOLD" | "UNSOLD" | "CANCELLED";
export type ItemStatus = "DRAFT" | "PENDING_AUDIT" | "REJECTED" | "READY_FOR_AUCTION" | "IN_AUCTION" | "SOLD" | "UNSOLD" | "OFFLINE";
export type OrderStatus = "PENDING_PAYMENT" | "PAID" | "WAITING_DELIVERY" | "SHIPPED" | "COMPLETED" | "REVIEWED" | "CLOSED";

export type AuctionItem = {
  id: string;
  title: string;
  category: string;
  imageUrl: string;
  currentPrice: number;
  startPrice: number;
  minIncrement: number;
  endTime: string;
  status: AuctionStatus;
  highestBidder: string;
  seller: {
    name: string;
    rating: number;
    deals: number;
  };
  watchers: number;
  location: string;
  tags: string[];
  description: string;
  tradeMode: string;
  sellerUsername?: string;
  acceptingBids?: boolean;
};

export type BidRecord = {
  id: string;
  auctionId: string;
  bidder: string;
  amount: number;
  createdAt: string;
  status: "SUCCESS" | "PENDING" | "OUTBID";
};

export type OrderSummary = {
  id: string;
  auctionTitle: string;
  hammerPrice: number;
  serviceFee: number;
  total: number;
  seller: string;
  deadline: string;
};

export type MyItem = {
  itemId: number;
  title: string;
  startPrice: number;
  status: ItemStatus;
  coverImageUrl: string;
  rejectReason: string;
  updatedAt: string;
};

export type UserOrder = {
  orderId: number;
  orderNo: string;
  auctionId: number;
  buyerId: number;
  sellerId: number;
  finalAmount: number;
  orderStatus: OrderStatus;
  payDeadlineAt: string;
  paidAt: string;
  shippedAt: string;
  completedAt: string;
  createdAt: string;
  itemTitle: string;
  coverImageUrl: string;
  buyerUsername: string;
  sellerUsername: string;
  latestPayment?: {
    paymentId: number;
    payStatus: string;
    payChannel: string;
    paidAt: string;
  } | null;
};

export type UserNotification = {
  notificationId: number;
  noticeType: string;
  title: string;
  content: string;
  bizType: string;
  bizId: number | null;
  readStatus: "UNREAD" | "READ";
  pushStatus: string;
  createdAt: string;
  readAt: string;
};

export type PaymentChannel = "MOCK_ALIPAY" | "MOCK_WECHAT";

export type AdminReviewItem = {
  id: string;
  title: string;
  seller: string;
  category: string;
  risk: "LOW" | "MEDIUM" | "HIGH";
  submittedAt: string;
  price: number;
  imageUrl?: string;
  location?: string;
  tradeMode?: string;
};

export type AdminReviewDecision = {
  itemId: string;
  title: string;
  seller: string;
  price: number;
  result: "APPROVED" | "REJECTED";
  auditedAt: string;
  comment?: string;
  auctionId?: string | null;
  auctionStatus?: string | null;
};

export type AuctionListQuery = {
  keyword?: string;
  status?: string;
  category?: string;
  priceMin?: number;
  priceMax?: number;
  sellerRating?: number;
  sellerHasDeals?: boolean;
  tradeMode?: string;
  pageNo?: number;
  pageSize?: number;
};

export type UserProfile = {
  user_id: number;
  username: string;
  nickname: string;
  role_code: string;
  status: string;
};

export type LoginResponse = {
  token: string;
  expire_at: string;
  user_info: UserProfile;
};

export type RegisterResponseRaw = {
  user_id: number;
  username: string;
  role_code: string;
  status: string;
};

export type AuctionSummaryRaw = {
  auction_id: number;
  item_id: number;
  title: string;
  category_name: string;
  cover_image_url: string;
  status: string;
  start_price: number;
  current_price: number;
  bid_step: number;
  seller_username: string;
  seller_rating: number;
  seller_deals: number;
  watcher_count: number;
  trade_mode: string;
  location: string;
  tags_json: string;
  description: string;
  start_time: string;
  end_time: string;
};

export type PublicAuctionSummaryRaw = {
  auctionId: number;
  itemId: number;
  title: string;
  categoryName?: string;
  coverImageUrl: string;
  status: string;
  startPrice?: number;
  currentPrice: number;
  bidStep?: number;
  sellerUsername?: string;
  sellerRating?: number;
  sellerDeals?: number;
  watcherCount?: number;
  tradeMode?: string;
  location?: string;
  tagsJson?: string;
  description?: string;
  startTime: string;
  endTime: string;
  acceptingBids?: boolean;
};

export type PublicAuctionListRaw = {
  list: PublicAuctionSummaryRaw[];
  total: number;
  pageNo: number;
  pageSize: number;
};

export type AuctionDetailRaw = AuctionSummaryRaw & {
  anti_sniping_window_seconds: number;
  extend_seconds: number;
  highest_bidder_masked: string;
};

export type PublicAuctionDetailRaw = {
  auctionId: number;
  itemId: number;
  title: string;
  categoryName?: string;
  coverImageUrl: string;
  status: string;
  startPrice: number;
  currentPrice: number;
  bidStep: number;
  sellerUsername?: string;
  sellerRating?: number;
  sellerDeals?: number;
  watcherCount?: number;
  tradeMode?: string;
  location?: string;
  tagsJson?: string;
  description?: string;
  startTime: string;
  endTime: string;
  antiSnipingWindowSeconds: number;
  extendSeconds: number;
  highestBidderMasked: string;
  acceptingBids?: boolean;
};

export type BidHistoryEntryRaw = {
  bid_id: number;
  bid_amount: number;
  bid_status: string;
  bid_time: string;
  bidder_masked: string;
};

export type BidHistoryResponseRaw = {
  records: BidHistoryEntryRaw[];
  page_no: number;
  page_size: number;
};

export type PublicBidHistoryEntryRaw = {
  bidId: number;
  bidAmount: number;
  bidStatus: string;
  bidTime: string;
  bidderMasked: string;
};

export type PublicBidHistoryResponseRaw = {
  list: PublicBidHistoryEntryRaw[];
  total: number;
  pageNo: number;
  pageSize: number;
};

export type PlaceBidResultRaw = {
  bid_id: number;
  auction_id: number;
  bid_amount: number;
  bid_status: string;
  current_price: number;
  highest_bidder_masked: string;
  end_time: string;
  extended: boolean;
  server_time: string;
};

export type CreateItemRaw = {
  item_id: number;
  item_status: string;
  created_at: string;
};

export type UpdateItemRaw = {
  item_id: number;
  item_status: string;
  updated_at: string;
};

export type SubmitReviewRaw = {
  item_id: number;
  item_status: string;
  submitted_at: string;
};

export type ItemSummaryRaw = {
  item_id: number;
  seller_id: number;
  category_id: number;
  title: string;
  start_price: number;
  item_status: ItemStatus;
  reject_reason: string;
  cover_image_url: string;
  updated_at: string;
};

export type MyItemsRaw = {
  items: ItemSummaryRaw[];
  total: number;
};

export type OfflineItemRaw = {
  item_id: number;
  item_status: ItemStatus;
  updated_at: string;
};

export type ItemImageRaw = {
  image_id: number;
  item_id: number;
  image_url: string;
  sort_no: number;
  is_cover: boolean;
  created_at: string;
};

export type OrderDetailRaw = {
  order_id: number;
  order_no: string;
  auction_id: number;
  item_title: string;
  buyer_id: number;
  seller_id: number;
  final_amount: number;
  order_status: string;
  pay_deadline_at: string;
  paid_at: string;
  created_at: string;
};

export type PayOrderResultRaw = {
  payment_id: number;
  order_id: number;
  order_no: string;
  payment_no: string;
  pay_channel: string;
  pay_amount: number;
  pay_status: string;
  pay_url: string;
  expire_at: string;
  reused_existing: boolean;
};

export type UserOrderListRaw = {
  records: UserOrder[];
  pageNo: number;
  pageSize: number;
};

export type OrderTransitionRaw = {
  order_id: number;
  old_status: string;
  new_status: OrderStatus;
};

export type ReviewRecordRaw = {
  reviewId: number;
  orderId: number;
  reviewerId: number;
  revieweeId: number;
  reviewType: string;
  rating: number;
  content: string;
  createdAt: string;
};

export type SubmitOrderReviewRaw = ReviewRecordRaw & {
  orderStatusAfter: OrderStatus;
  orderMarkedReviewed: boolean;
};

export type OrderReviewListRaw = {
  orderId: number;
  orderStatus: OrderStatus;
  reviews: ReviewRecordRaw[];
  total: number;
};

export type NotificationListRaw = {
  list: UserNotification[];
  total: number;
  unreadCount: number;
  limit: number;
};

export type MarkNotificationReadRaw = {
  notificationId: number;
  readStatus: "READ";
  readAt: string;
};

export type OperationLogRaw = {
  operationLogId: number;
  moduleName: string;
  operationName: string;
  bizKey: string;
  result: string;
  detail: string;
  createdAt: string;
};

export type TaskLogRaw = {
  taskLogId: number;
  taskType: string;
  bizKey: string;
  taskStatus: string;
  retryCount: number;
  lastError: string;
  createdAt: string;
};

export type SystemExceptionRaw = {
  sourceType: string;
  sourceId: string;
  bizKey: string;
  currentStatus: string;
  summary: string;
  detail: string;
  occurredAt: string;
  retryable: boolean;
};

export type OpsListRaw<T> = {
  list: T[];
  total: number;
};

export type OpsActionResultRaw = {
  scanned: number;
  succeeded: number;
  failed: number;
  skipped: number;
};

export type PendingAuditItemRaw = {
  item_id: number;
  seller_id: number;
  seller_username: string;
  seller_nickname: string;
  category_id: number;
  title: string;
  start_price: number;
  cover_image_url: string;
  created_at: string;
};

export type AuditItemResultRaw = {
  item_id: number;
  old_status: string;
  new_status: string;
  audit_result: string;
  audited_at: string;
};

export type DailyStatisticsRaw = {
  stat_id: number;
  stat_date: string;
  auction_count: number;
  sold_count: number;
  unsold_count: number;
  bid_count: number;
  gmv_amount: number;
  created_at: string;
};

export type AdminAuctionSummaryRaw = {
  auctionId: number;
  itemId: number;
  sellerId: number;
  title: string;
  coverImageUrl: string;
  status: string;
  startPrice: number;
  currentPrice: number;
  bidStep: number;
  highestBidderId: number | null;
  startTime: string;
  endTime: string;
  updatedAt: string;
};

export type AdminAuctionListRaw = {
  list: AdminAuctionSummaryRaw[];
  total: number;
  pageNo: number;
  pageSize: number;
};

export type CreateAdminAuctionResultRaw = {
  auctionId: number;
  itemId: number;
  status: string;
  currentPrice: number;
  createdAt: string;
};
