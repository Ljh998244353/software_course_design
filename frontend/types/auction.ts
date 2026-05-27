export type AuctionStatus = "PENDING_START" | "RUNNING" | "SETTLING" | "SOLD" | "UNSOLD";

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
