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
