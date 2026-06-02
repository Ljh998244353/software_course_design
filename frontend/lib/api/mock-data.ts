import type { AdminReviewItem, AuctionItem, BidRecord, OrderSummary } from "@/types/auction";

const future = (minutes: number) => new Date(Date.now() + minutes * 60000).toISOString();
const past = (minutes: number) => new Date(Date.now() - minutes * 60000).toISOString();

export const mockAuctions: AuctionItem[] = [
  {
    id: "AUC-1001",
    title: "索尼 WH-1000XM5 降噪耳机",
    category: "数码3C",
    imageUrl: "https://images.unsplash.com/photo-1618366712010-f4ae9c647dcb?auto=format&fit=crop&w=900&q=80",
    currentPrice: 860,
    startPrice: 600,
    minIncrement: 20,
    endTime: future(216),
    status: "RUNNING",
    highestBidder: "buyer_demo",
    seller: { name: "seller_demo", rating: 4.9, deals: 28 },
    watchers: 143,
    location: "犀浦校区",
    tags: ["99新", "带发票", "可当面验机"],
    description: "通勤和自习室常用降噪耳机，电池健康稳定，外壳轻微使用痕迹。",
    tradeMode: "MEETUP",
    sellerUsername: "seller_demo"
  },
  {
    id: "AUC-1002",
    title: "佳能 EOS M50 Mark II 套机",
    category: "数码3C",
    imageUrl: "https://images.unsplash.com/photo-1510127034890-ba27508e9f1c?auto=format&fit=crop&w=900&q=80",
    currentPrice: 3180,
    startPrice: 2600,
    minIncrement: 50,
    endTime: future(18),
    status: "RUNNING",
    highestBidder: "buyer_demo_2",
    seller: { name: "film_room", rating: 4.8, deals: 17 },
    watchers: 219,
    location: "九里校区",
    tags: ["套机", "适合Vlog", "临近截标"],
    description: "含机身、15-45 镜头、电池、充电器和 64G 卡，适合课程拍摄。",
    tradeMode: "MEETUP",
    sellerUsername: "film_room"
  },
  {
    id: "AUC-1003",
    title: "捷安特 Escape 1 校园通勤车",
    category: "运动装备",
    imageUrl: "https://images.unsplash.com/photo-1485965120184-e220f721d03e?auto=format&fit=crop&w=900&q=80",
    currentPrice: 1280,
    startPrice: 900,
    minIncrement: 30,
    endTime: future(1440),
    status: "RUNNING",
    highestBidder: "cycle_lab",
    seller: { name: "rider_x", rating: 4.7, deals: 12 },
    watchers: 87,
    location: "唐山路宿舍区",
    tags: ["保养完成", "可试骑"],
    description: "M 码车架，刚做过刹车和变速调校，适合日常通勤。",
    tradeMode: "SELF_PICKUP",
    sellerUsername: "rider_x"
  },
  {
    id: "AUC-1004",
    title: "软件工程导论与 UML 套装教材",
    category: "图书教材",
    imageUrl: "https://images.unsplash.com/photo-1495446815901-a7297e633e8d?auto=format&fit=crop&w=900&q=80",
    currentPrice: 96,
    startPrice: 60,
    minIncrement: 5,
    endTime: future(910),
    status: "RUNNING",
    highestBidder: "book_club",
    seller: { name: "senior_2026", rating: 5.0, deals: 34 },
    watchers: 53,
    location: "图书馆北门",
    tags: ["笔记完整", "课程设计参考"],
    description: "包含软件工程导论、大象 UML 等课程常用资料，附课堂整理笔记。",
    tradeMode: "MEETUP",
    sellerUsername: "senior_2026"
  },
  {
    id: "AUC-1005",
    title: "人体工学升降桌 120cm",
    category: "生活闲置",
    imageUrl: "https://images.unsplash.com/photo-1518455027359-f3f8164ba6bd?auto=format&fit=crop&w=900&q=80",
    currentPrice: 420,
    startPrice: 300,
    minIncrement: 20,
    endTime: future(52),
    status: "RUNNING",
    highestBidder: "desk_user",
    seller: { name: "studio_mate", rating: 4.6, deals: 9 },
    watchers: 74,
    location: "创新港",
    tags: ["自提", "稳定承重"],
    description: "桌面有轻微划痕，电机升降正常，适合宿舍或实验室工位。",
    tradeMode: "SELF_PICKUP",
    sellerUsername: "studio_mate"
  },
  {
    id: "AUC-1006",
    title: "iPad Air 5 64G 蓝色",
    category: "数码3C",
    imageUrl: "https://images.unsplash.com/photo-1544244015-0df4b3ffc6b0?auto=format&fit=crop&w=900&q=80",
    currentPrice: 2920,
    startPrice: 2300,
    minIncrement: 50,
    endTime: future(6),
    status: "RUNNING",
    highestBidder: "buyer_demo",
    seller: { name: "design_lab", rating: 4.9, deals: 21 },
    watchers: 301,
    location: "犀浦校区",
    tags: ["小于5分钟", "带壳膜", "高热度"],
    description: "屏幕无划痕，主要用于阅读论文和手写笔记，不含 Apple Pencil。",
    tradeMode: "MEETUP",
    sellerUsername: "design_lab"
  }
];

export const mockBids: BidRecord[] = [
  { id: "BID-1", auctionId: "AUC-1001", bidder: "buyer_demo", amount: 860, createdAt: past(2), status: "SUCCESS" },
  { id: "BID-2", auctionId: "AUC-1001", bidder: "buyer_demo_2", amount: 840, createdAt: past(5), status: "OUTBID" },
  { id: "BID-3", auctionId: "AUC-1001", bidder: "campus_user", amount: 820, createdAt: past(8), status: "OUTBID" },
  { id: "BID-4", auctionId: "AUC-1001", bidder: "freshman_7", amount: 780, createdAt: past(12), status: "OUTBID" }
];

export const mockOrder: OrderSummary = {
  id: "ORD-20260527-1001",
  auctionTitle: "索尼 WH-1000XM5 降噪耳机",
  hammerPrice: 860,
  serviceFee: 0,
  total: 860,
  seller: "seller_demo",
  deadline: future(1440)
};

export const mockReviewItems: AdminReviewItem[] = [
  { id: "ITM-9101", title: "MacBook Air M2 16G", seller: "seller_demo", category: "数码3C", risk: "LOW", submittedAt: past(20), price: 5200, location: "犀浦校区", tradeMode: "MEETUP" },
  { id: "ITM-9102", title: "实验室闲置路由器套装", seller: "network_lab", category: "生活闲置", risk: "MEDIUM", submittedAt: past(44), price: 180, location: "九里校区", tradeMode: "SELF_PICKUP" },
  { id: "ITM-9103", title: "限量球鞋未拆封", seller: "sneaker_x", category: "运动装备", risk: "HIGH", submittedAt: past(63), price: 900, location: "创新港", tradeMode: "MEETUP" }
];
