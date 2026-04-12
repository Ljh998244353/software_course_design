CREATE TABLE IF NOT EXISTS user_account (
    user_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    phone VARCHAR(20) DEFAULT NULL,
    email VARCHAR(100) DEFAULT NULL,
    role_code VARCHAR(20) NOT NULL,
    status VARCHAR(20) NOT NULL,
    nickname VARCHAR(50) DEFAULT NULL,
    last_login_at DATETIME(3) DEFAULT NULL,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (user_id),
    UNIQUE KEY uk_user_account_username (username),
    UNIQUE KEY uk_user_account_phone (phone),
    UNIQUE KEY uk_user_account_email (email),
    KEY idx_user_account_role_status (role_code, status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS item_category (
    category_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    category_name VARCHAR(64) NOT NULL,
    status VARCHAR(20) NOT NULL,
    sort_no INT NOT NULL DEFAULT 0,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (category_id),
    UNIQUE KEY uk_item_category_name (category_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS item (
    item_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    seller_id BIGINT UNSIGNED NOT NULL,
    category_id BIGINT UNSIGNED DEFAULT NULL,
    title VARCHAR(200) NOT NULL,
    description TEXT NOT NULL,
    start_price DECIMAL(12, 2) NOT NULL,
    item_status VARCHAR(32) NOT NULL,
    reject_reason VARCHAR(255) DEFAULT NULL,
    cover_image_url VARCHAR(255) DEFAULT NULL,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (item_id),
    KEY idx_item_seller_status_created (seller_id, item_status, created_at),
    KEY idx_item_category_status (category_id, item_status),
    CONSTRAINT fk_item_seller FOREIGN KEY (seller_id) REFERENCES user_account (user_id),
    CONSTRAINT fk_item_category FOREIGN KEY (category_id) REFERENCES item_category (category_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS item_image (
    image_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    item_id BIGINT UNSIGNED NOT NULL,
    image_url VARCHAR(255) NOT NULL,
    sort_no INT NOT NULL DEFAULT 0,
    is_cover TINYINT(1) NOT NULL DEFAULT 0,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    PRIMARY KEY (image_id),
    UNIQUE KEY uk_item_image_item_sort (item_id, sort_no),
    KEY idx_item_image_item_cover (item_id, is_cover),
    CONSTRAINT fk_item_image_item FOREIGN KEY (item_id) REFERENCES item (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS item_audit_log (
    audit_log_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    item_id BIGINT UNSIGNED NOT NULL,
    admin_id BIGINT UNSIGNED NOT NULL,
    audit_result VARCHAR(20) NOT NULL,
    audit_comment VARCHAR(255) DEFAULT NULL,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    PRIMARY KEY (audit_log_id),
    KEY idx_item_audit_log_item_created (item_id, created_at),
    KEY idx_item_audit_log_admin_created (admin_id, created_at),
    CONSTRAINT fk_item_audit_log_item FOREIGN KEY (item_id) REFERENCES item (item_id),
    CONSTRAINT fk_item_audit_log_admin FOREIGN KEY (admin_id) REFERENCES user_account (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS auction (
    auction_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    item_id BIGINT UNSIGNED NOT NULL,
    seller_id BIGINT UNSIGNED NOT NULL,
    start_time DATETIME(3) NOT NULL,
    end_time DATETIME(3) NOT NULL,
    start_price DECIMAL(12, 2) NOT NULL,
    current_price DECIMAL(12, 2) NOT NULL,
    bid_step DECIMAL(12, 2) NOT NULL,
    highest_bidder_id BIGINT UNSIGNED DEFAULT NULL,
    status VARCHAR(20) NOT NULL,
    anti_sniping_window_seconds INT NOT NULL DEFAULT 0,
    extend_seconds INT NOT NULL DEFAULT 0,
    version BIGINT UNSIGNED NOT NULL DEFAULT 0,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (auction_id),
    KEY idx_auction_status_start_end (status, start_time, end_time),
    KEY idx_auction_item_status (item_id, status),
    KEY idx_auction_seller_status (seller_id, status),
    KEY idx_auction_highest_bidder (highest_bidder_id),
    CONSTRAINT fk_auction_item FOREIGN KEY (item_id) REFERENCES item (item_id),
    CONSTRAINT fk_auction_seller FOREIGN KEY (seller_id) REFERENCES user_account (user_id),
    CONSTRAINT fk_auction_highest_bidder FOREIGN KEY (highest_bidder_id) REFERENCES user_account (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS bid_record (
    bid_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    auction_id BIGINT UNSIGNED NOT NULL,
    bidder_id BIGINT UNSIGNED NOT NULL,
    request_id VARCHAR(64) NOT NULL,
    bid_amount DECIMAL(12, 2) NOT NULL,
    bid_status VARCHAR(20) NOT NULL,
    bid_time DATETIME(3) NOT NULL,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    PRIMARY KEY (bid_id),
    UNIQUE KEY uk_bid_record_request (auction_id, bidder_id, request_id),
    KEY idx_bid_record_auction_created (auction_id, created_at),
    KEY idx_bid_record_auction_bidder_created (auction_id, bidder_id, created_at),
    CONSTRAINT fk_bid_record_auction FOREIGN KEY (auction_id) REFERENCES auction (auction_id),
    CONSTRAINT fk_bid_record_bidder FOREIGN KEY (bidder_id) REFERENCES user_account (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS order_info (
    order_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    order_no VARCHAR(64) NOT NULL,
    auction_id BIGINT UNSIGNED NOT NULL,
    buyer_id BIGINT UNSIGNED NOT NULL,
    seller_id BIGINT UNSIGNED NOT NULL,
    final_amount DECIMAL(12, 2) NOT NULL,
    order_status VARCHAR(20) NOT NULL,
    pay_deadline_at DATETIME(3) DEFAULT NULL,
    paid_at DATETIME(3) DEFAULT NULL,
    shipped_at DATETIME(3) DEFAULT NULL,
    completed_at DATETIME(3) DEFAULT NULL,
    closed_at DATETIME(3) DEFAULT NULL,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (order_id),
    UNIQUE KEY uk_order_info_order_no (order_no),
    UNIQUE KEY uk_order_info_auction (auction_id),
    KEY idx_order_info_buyer_status (buyer_id, order_status),
    KEY idx_order_info_seller_status (seller_id, order_status),
    CONSTRAINT fk_order_info_auction FOREIGN KEY (auction_id) REFERENCES auction (auction_id),
    CONSTRAINT fk_order_info_buyer FOREIGN KEY (buyer_id) REFERENCES user_account (user_id),
    CONSTRAINT fk_order_info_seller FOREIGN KEY (seller_id) REFERENCES user_account (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS payment_record (
    payment_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    payment_no VARCHAR(64) NOT NULL,
    order_id BIGINT UNSIGNED NOT NULL,
    pay_channel VARCHAR(20) NOT NULL,
    transaction_no VARCHAR(64) DEFAULT NULL,
    pay_amount DECIMAL(12, 2) NOT NULL,
    pay_status VARCHAR(20) NOT NULL,
    callback_payload_hash VARCHAR(128) DEFAULT NULL,
    paid_at DATETIME(3) DEFAULT NULL,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (payment_id),
    UNIQUE KEY uk_payment_record_payment_no (payment_no),
    UNIQUE KEY uk_payment_record_transaction_no (transaction_no),
    KEY idx_payment_record_order_status (order_id, pay_status),
    CONSTRAINT fk_payment_record_order FOREIGN KEY (order_id) REFERENCES order_info (order_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS payment_callback_log (
    callback_log_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    payment_id BIGINT UNSIGNED DEFAULT NULL,
    order_id BIGINT UNSIGNED DEFAULT NULL,
    transaction_no VARCHAR(64) DEFAULT NULL,
    callback_status VARCHAR(20) NOT NULL,
    raw_payload LONGTEXT NOT NULL,
    received_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    processed_at DATETIME(3) DEFAULT NULL,
    process_result VARCHAR(255) DEFAULT NULL,
    PRIMARY KEY (callback_log_id),
    KEY idx_payment_callback_log_tx_no (transaction_no, received_at),
    KEY idx_payment_callback_log_order_received (order_id, received_at),
    CONSTRAINT fk_payment_callback_log_payment FOREIGN KEY (payment_id) REFERENCES payment_record (payment_id),
    CONSTRAINT fk_payment_callback_log_order FOREIGN KEY (order_id) REFERENCES order_info (order_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS review (
    review_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    order_id BIGINT UNSIGNED NOT NULL,
    reviewer_id BIGINT UNSIGNED NOT NULL,
    reviewee_id BIGINT UNSIGNED NOT NULL,
    review_type VARCHAR(32) NOT NULL,
    rating TINYINT UNSIGNED NOT NULL,
    content VARCHAR(500) DEFAULT NULL,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    PRIMARY KEY (review_id),
    UNIQUE KEY uk_review_order_reviewer (order_id, reviewer_id),
    KEY idx_review_reviewee_created (reviewee_id, created_at),
    CONSTRAINT fk_review_order FOREIGN KEY (order_id) REFERENCES order_info (order_id),
    CONSTRAINT fk_review_reviewer FOREIGN KEY (reviewer_id) REFERENCES user_account (user_id),
    CONSTRAINT fk_review_reviewee FOREIGN KEY (reviewee_id) REFERENCES user_account (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS notification (
    notification_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    user_id BIGINT UNSIGNED NOT NULL,
    notice_type VARCHAR(32) NOT NULL,
    title VARCHAR(100) NOT NULL,
    content VARCHAR(1000) NOT NULL,
    biz_type VARCHAR(32) DEFAULT NULL,
    biz_id BIGINT UNSIGNED DEFAULT NULL,
    read_status VARCHAR(20) NOT NULL,
    push_status VARCHAR(20) NOT NULL,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    read_at DATETIME(3) DEFAULT NULL,
    PRIMARY KEY (notification_id),
    KEY idx_notification_user_read_created (user_id, read_status, created_at),
    KEY idx_notification_biz_type_id (biz_type, biz_id),
    CONSTRAINT fk_notification_user FOREIGN KEY (user_id) REFERENCES user_account (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS statistics_daily (
    stat_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    stat_date DATE NOT NULL,
    auction_count INT NOT NULL DEFAULT 0,
    sold_count INT NOT NULL DEFAULT 0,
    unsold_count INT NOT NULL DEFAULT 0,
    bid_count INT NOT NULL DEFAULT 0,
    gmv_amount DECIMAL(12, 2) NOT NULL DEFAULT 0.00,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
    PRIMARY KEY (stat_id),
    UNIQUE KEY uk_statistics_daily_date (stat_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS task_log (
    task_log_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    task_type VARCHAR(64) NOT NULL,
    biz_key VARCHAR(128) NOT NULL,
    task_status VARCHAR(20) NOT NULL,
    retry_count INT NOT NULL DEFAULT 0,
    last_error VARCHAR(255) DEFAULT NULL,
    scheduled_at DATETIME(3) DEFAULT NULL,
    started_at DATETIME(3) DEFAULT NULL,
    finished_at DATETIME(3) DEFAULT NULL,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    PRIMARY KEY (task_log_id),
    KEY idx_task_log_type_status (task_type, task_status),
    KEY idx_task_log_biz_key (biz_key)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE IF NOT EXISTS operation_log (
    operation_log_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    operator_id BIGINT UNSIGNED DEFAULT NULL,
    operator_role VARCHAR(20) NOT NULL,
    module_name VARCHAR(64) NOT NULL,
    operation_name VARCHAR(64) NOT NULL,
    biz_key VARCHAR(128) DEFAULT NULL,
    result VARCHAR(20) NOT NULL,
    detail TEXT DEFAULT NULL,
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    PRIMARY KEY (operation_log_id),
    KEY idx_operation_log_operator_created (operator_id, created_at),
    KEY idx_operation_log_module_created (module_name, created_at),
    CONSTRAINT fk_operation_log_operator FOREIGN KEY (operator_id) REFERENCES user_account (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
