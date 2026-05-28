export type AuctionStatus = "PENDING_START" | "RUNNING" | "SETTLING" | "SOLD" | "UNSOLD" | "CANCELLED";

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

export type PaymentChannel = "MOCK_ALIPAY" | "MOCK_WECHAT";

export type AdminReviewItem = {
  id: string;
  title: string;
  seller: string;
  category: string;
  risk: "LOW" | "MEDIUM" | "HIGH";
  submittedAt: string;
  price: number;
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

export type AuctionSummaryRaw = {
  auction_id: number;
  item_id: number;
  title: string;
  cover_image_url: string;
  status: string;
  current_price: number;
  start_time: string;
  end_time: string;
};

export type AuctionDetailRaw = AuctionSummaryRaw & {
  start_price: number;
  bid_step: number;
  anti_sniping_window_seconds: number;
  extend_seconds: number;
  highest_bidder_masked: string;
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

export type PendingAuditItemRaw = {
  item_id: number;
  seller_id: number;
  seller_username: string;
  seller_nickname: string;
  category_id: number;
  title: string;
  start_price: number;
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
