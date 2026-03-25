# Auction System API Specification

## Base URL
```
http://localhost:8080/api
```

## Authentication

All authenticated endpoints require the `Authorization` header:
```
Authorization: Bearer <jwt_token>
```

## Response Format

### Success Response
```json
{
  "code": 0,
  "message": "success",
  "data": { ... }
}
```

### Error Response
```json
{
  "code": <error_code>,
  "message": "<error_message>",
  "data": null
}
```

## Error Codes

| Code | Description |
|------|-------------|
| 0 | Success |
| 10001 | Invalid parameters |
| 10002 | Not authenticated |
| 10003 | Insufficient permissions |
| 10004 | Resource not found |
| 20001 | Bid amount too low |
| 20002 | Auction has ended |
| 20003 | Self-bidding not allowed |
| 20004 | Insufficient balance |
| 30001 | Order not found |
| 30002 | Payment failed |
| 30003 | Order already paid |
| 40001 | Item not found |
| 40002 | Item status not allowed |

## Endpoints

### Health Check

#### GET /api/health
Check server health status.

**Response:**
```json
{
  "code": 0,
  "data": {
    "status": "ok",
    "version": "1.0.0"
  }
}
```

### Authentication

#### POST /api/auth/register
Register a new user.

**Request Body:**
```json
{
  "username": "string (3-32 chars)",
  "password": "string (6-32 chars)",
  "email": "string (email format)",
  "phone": "string (optional)"
}
```

**Response:**
```json
{
  "code": 0,
  "data": {
    "user_id": 123,
    "username": "testuser"
  }
}
```

#### POST /api/auth/login
User login.

**Request Body:**
```json
{
  "username": "string",
  "password": "string"
}
```

**Response:**
```json
{
  "code": 0,
  "data": {
    "token": "eyJhbGciOiJIUzI1NiIs...",
    "expires_in": 86400,
    "user": {
      "id": 123,
      "username": "testuser",
      "role": "user"
    }
  }
}
```

### Users

#### GET /api/users/me
Get current user info.

**Response:**
```json
{
  "code": 0,
  "data": {
    "id": 123,
    "username": "testuser",
    "email": "test@example.com",
    "balance": "1000.00",
    "credit_score": "4.8"
  }
}
```

### Items

#### POST /api/items
Create a new item (requires authentication).

**Request Body:**
```json
{
  "title": "iPhone 15 Pro",
  "description": "99新无划痕",
  "category_id": 1,
  "start_price": 5000.00,
  "reserve_price": 4500.00,
  "bid_step": 50.00,
  "images": ["https://example.com/image1.jpg"]
}
```

**Response:**
```json
{
  "code": 0,
  "data": {
    "id": 1,
    "title": "iPhone 15 Pro",
    "status": "draft"
  }
}
```

#### GET /api/items/{id}
Get item details.

**Response:**
```json
{
  "code": 0,
  "data": {
    "id": 1,
    "title": "iPhone 15 Pro",
    "description": "99新无划痕",
    "category": {"id": 1, "name": "数码"},
    "start_price": "5000.00",
    "current_price": "5500.00",
    "bid_step": "50.00",
    "seller": {"id": 10, "username": "seller1"},
    "status": "active",
    "images": ["..."],
    "created_at": "2026-03-20T10:00:00Z"
  }
}
```

#### GET /api/items
List items with pagination.

**Query Parameters:**
- `status`: Item status (draft/pending_review/active/closed/rejected)
- `category_id`: Category ID
- `page`: Page number (default: 1)
- `page_size`: Items per page (default: 20, max: 100)
- `keyword`: Search keyword

### Auctions

#### POST /api/auctions
Create auction (admin only).

**Request Body:**
```json
{
  "name": "春季数码专场",
  "description": "全场数码产品低价起拍",
  "start_time": "2026-04-01T00:00:00Z",
  "end_time": "2026-04-07T23:59:59Z",
  "anti_snipe_N": 60,
  "anti_snipe_M": 120,
  "max_extensions": 10,
  "rule": {
    "min_bid_count": 1,
    "auto_relist": false
  }
}
```

#### GET /api/auctions
List auctions.

**Query Parameters:**
- `status`: Auction status (planned/running/ended/cancelled)
- `page`: Page number
- `page_size`: Items per page

#### POST /api/auctions/{id}/items
Add item to auction.

**Request Body:**
```json
{
  "item_id": 123
}
```

### Bidding

#### POST /api/bids
Submit a bid.

**Request Body:**
```json
{
  "auction_item_id": 123,
  "amount": 5500.00,
  "idempotent_token": "uuid-v4"
}
```

**Response (Success):**
```json
{
  "code": 0,
  "data": {
    "bid_id": 456,
    "current_price": "5500.00",
    "highest_bidder_id": 789,
    "bid_count": 42,
    "next_end_time": "2026-03-25T16:00:00Z",
    "is_extended": false
  }
}
```

**Response (Anti-Snipe Triggered):**
```json
{
  "code": 0,
  "data": {
    "bid_id": 456,
    "current_price": "5500.00",
    "highest_bidder_id": 789,
    "bid_count": 43,
    "next_end_time": "2026-03-25T16:02:00Z",
    "is_extended": true,
    "extension_count": 2
  }
}
```

#### GET /api/auction-items/{id}/bids
Get bid history.

**Query Parameters:**
- `page`: Page number
- `page_size`: Items per page

### Orders

#### GET /api/orders
Get user's orders.

**Query Parameters:**
- `status`: Order status
- `role`: buyer/seller
- `page`: Page number

#### POST /api/orders/{id}/pay
Initiate payment.

**Request Body:**
```json
{
  "method": "alipay",
  "idempotent_token": "uuid-v4"
}
```

#### POST /api/orders/{id}/ship
Mark order as shipped (seller).

**Request Body:**
```json
{
  "tracking_no": "SF1234567890",
  "carrier": "顺丰速运"
}
```

#### POST /api/orders/{id}/confirm
Confirm order received (buyer).

### Comments

#### POST /api/orders/{id}/comment
Submit comment for order.

**Request Body:**
```json
{
  "rating": 5,
  "content": "物品很好，卖家发货快！"
}
```

## WebSocket

### Connection
```
wss://api.example.com/ws?token=<jwt_token>
```

### Message Format

**Client to Server:**

Subscribe to auction items:
```json
{
  "type": "subscribe",
  "auction_item_ids": [123, 456]
}
```

Heartbeat:
```json
{
  "type": "ping"
}
```

**Server to Client:**

Bid Update:
```json
{
  "type": "bid_update",
  "data": {
    "auction_item_id": 123,
    "current_price": 1500.00,
    "highest_bidder_id": 789,
    "bid_count": 42,
    "next_end_time": "2026-03-25T15:30:00Z"
  }
}
```

Auction Ended:
```json
{
  "type": "auction_ended",
  "data": {
    "auction_item_id": 123,
    "final_price": 1500.00,
    "winner_id": 789,
    "status": "sold"
  }
}
```

Countdown Tick:
```json
{
  "type": "countdown_tick",
  "data": {
    "auction_item_id": 123,
    "remaining_seconds": 60,
    "is_extended": false
  }
}
```
