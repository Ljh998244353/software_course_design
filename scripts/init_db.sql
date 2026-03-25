CREATE DATABASE IF NOT EXISTS auction_db DEFAULT CHARSET=utf8mb4;
USE auction_db;

CREATE TABLE users (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  username VARCHAR(64) UNIQUE NOT NULL,
  email VARCHAR(128),
  phone VARCHAR(32),
  password_hash VARCHAR(255) NOT NULL,
  role ENUM('user','seller','admin') DEFAULT 'user',
  real_name VARCHAR(128),
  avatar_url VARCHAR(512),
  verified BOOLEAN DEFAULT FALSE,
  balance DECIMAL(12,2) DEFAULT 0.00,
  frozen_balance DECIMAL(12,2) DEFAULT 0.00,
  credit_score DECIMAL(3,1) DEFAULT 5.0,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_username (username),
  INDEX idx_email (email),
  INDEX idx_role (role)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE categories (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(64) NOT NULL,
  parent_id INT DEFAULT NULL,
  FOREIGN KEY (parent_id) REFERENCES categories(id),
  INDEX idx_parent (parent_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE items (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  seller_id BIGINT NOT NULL,
  category_id INT,
  title VARCHAR(200) NOT NULL,
  description TEXT,
  start_price DECIMAL(12,2) NOT NULL,
  reserve_price DECIMAL(12,2),
  bid_step DECIMAL(12,2) NOT NULL DEFAULT 1.00,
  images JSON,
  status ENUM('draft','pending_review','active','closed','rejected') DEFAULT 'draft',
  version INT DEFAULT 0,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (seller_id) REFERENCES users(id),
  INDEX idx_seller (seller_id),
  INDEX idx_status (status),
  INDEX idx_category (category_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE auctions (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(200) NOT NULL,
  description TEXT,
  start_time DATETIME NOT NULL,
  end_time DATETIME NOT NULL,
  anti_snipe_N INT DEFAULT 60,
  anti_snipe_M INT DEFAULT 120,
  max_extensions INT DEFAULT 10,
  rule JSON,
  status ENUM('planned','running','ended','cancelled') DEFAULT 'planned',
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_status (status),
  INDEX idx_time (start_time, end_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE auction_items (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  auction_id BIGINT NOT NULL,
  item_id BIGINT NOT NULL UNIQUE,
  current_price DECIMAL(12,2) NOT NULL,
  highest_bidder_id BIGINT,
  bid_count INT DEFAULT 0,
  extension_count INT DEFAULT 0,
  status ENUM('waiting','on_auction','sold','unsold') DEFAULT 'waiting',
  version INT DEFAULT 0,
  FOREIGN KEY (auction_id) REFERENCES auctions(id),
  FOREIGN KEY (item_id) REFERENCES items(id),
  INDEX idx_auction (auction_id),
  INDEX idx_status (status),
  INDEX idx_highest_bidder (highest_bidder_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE bids (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  auction_item_id BIGINT NOT NULL,
  bidder_id BIGINT NOT NULL,
  amount DECIMAL(12,2) NOT NULL,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  idempotent_token VARCHAR(64),
  FOREIGN KEY (auction_item_id) REFERENCES auction_items(id),
  FOREIGN KEY (bidder_id) REFERENCES users(id),
  UNIQUE INDEX idx_idempotent (idempotent_token),
  INDEX idx_item_bidder (auction_item_id, bidder_id),
  INDEX idx_item_amount (auction_item_id, amount),
  INDEX idx_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE orders (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  order_no VARCHAR(32) UNIQUE NOT NULL,
  auction_item_id BIGINT NOT NULL,
  buyer_id BIGINT NOT NULL,
  seller_id BIGINT NOT NULL,
  final_price DECIMAL(12,2) NOT NULL,
  commission DECIMAL(12,2) DEFAULT 0.00,
  status ENUM('pending_payment','paid','shipped','completed','cancelled','timeout') DEFAULT 'pending_payment',
  payment_deadline DATETIME,
  paid_at DATETIME,
  shipped_at DATETIME,
  completed_at DATETIME,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (auction_item_id) REFERENCES auction_items(id),
  FOREIGN KEY (buyer_id) REFERENCES users(id),
  FOREIGN KEY (seller_id) REFERENCES users(id),
  INDEX idx_buyer (buyer_id),
  INDEX idx_seller (seller_id),
  INDEX idx_status (status),
  INDEX idx_payment_deadline (payment_deadline)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE payments (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  order_id BIGINT NOT NULL,
  amount DECIMAL(12,2) NOT NULL,
  method ENUM('alipay','wechat','card') DEFAULT 'alipay',
  status ENUM('initiated','success','failed','refunded') DEFAULT 'initiated',
  txn_id VARCHAR(128),
  idempotent_token VARCHAR(64) UNIQUE,
  callback_data JSON,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (order_id) REFERENCES orders(id),
  INDEX idx_order (order_id),
  INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE comments (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  order_id BIGINT NOT NULL,
  from_user_id BIGINT NOT NULL,
  to_user_id BIGINT NOT NULL,
  rating TINYINT NOT NULL CHECK (rating >= 1 AND rating <= 5),
  content TEXT,
  is_hidden BOOLEAN DEFAULT FALSE,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (order_id) REFERENCES orders(id),
  FOREIGN KEY (from_user_id) REFERENCES users(id),
  FOREIGN KEY (to_user_id) REFERENCES users(id),
  INDEX idx_to_user (to_user_id),
  INDEX idx_order (order_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Insert default categories
INSERT INTO categories (name, parent_id) VALUES
('数码', NULL),
('手机', 1),
('电脑', 1),
('二手汽车', NULL),
('图书', NULL),
('艺术品', NULL);
