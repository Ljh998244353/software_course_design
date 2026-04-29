SET @demo_admin_username := 'admin';
SET @demo_support_username := 'support';
SET @demo_seller_username := 'seller_demo';
SET @demo_buyer_username := 'buyer_demo';
SET @demo_buyer_2_username := 'buyer_demo_2';

SET FOREIGN_KEY_CHECKS = 0;

DELETE FROM review
WHERE order_id IN (
    SELECT order_id FROM order_info WHERE order_no LIKE 'DEMO-%'
);

DELETE FROM payment_callback_log
WHERE order_id IN (
    SELECT order_id FROM order_info WHERE order_no LIKE 'DEMO-%'
);

DELETE FROM payment_record
WHERE order_id IN (
    SELECT order_id FROM order_info WHERE order_no LIKE 'DEMO-%'
);

DELETE FROM notification
WHERE (biz_type = 'ORDER' AND biz_id IN (
    SELECT order_id FROM order_info WHERE order_no LIKE 'DEMO-%'
)) OR (biz_type = 'AUCTION' AND biz_id IN (
    SELECT auction_id FROM auction WHERE item_id IN (
        SELECT item_id FROM item WHERE title LIKE '演示%'
    )
));

DELETE FROM order_info
WHERE order_no LIKE 'DEMO-%';

DELETE FROM bid_record
WHERE auction_id IN (
    SELECT auction_id FROM auction WHERE item_id IN (
        SELECT item_id FROM item WHERE title LIKE '演示%'
    )
);

DELETE FROM task_log
WHERE biz_key LIKE 'DEMO-%';

DELETE FROM operation_log
WHERE biz_key LIKE 'DEMO-%' OR detail LIKE '%DEMO-%';

DELETE FROM auction
WHERE item_id IN (
    SELECT item_id FROM item WHERE title LIKE '演示%'
);

DELETE FROM item_audit_log
WHERE item_id IN (
    SELECT item_id FROM item WHERE title LIKE '演示%'
);

DELETE FROM item_image
WHERE item_id IN (
    SELECT item_id FROM item WHERE title LIKE '演示%'
);

DELETE FROM item
WHERE title LIKE '演示%';

SET FOREIGN_KEY_CHECKS = 1;

INSERT INTO user_account (
    username,
    password_hash,
    phone,
    email,
    role_code,
    status,
    nickname
) VALUES
    (
        @demo_admin_username,
        '$6$demoAdmin2026$rgv/K.aXsb0CNdspt5F/CuvYAaPrV.7AAL5xN7kct3xU2IbtDjUmWIpNLTchuAPhUb1YFe2bxl0rMeL9mLj.Z/',
        '18800000001',
        'admin@auction.local',
        'ADMIN',
        'ACTIVE',
        '平台管理员'
    ),
    (
        @demo_support_username,
        '$6$demoSupport2026$jKTOCHjIrzq99uj6J9EphGr1md8sKjFOLmCrcn8vaUfjzV7dvEPHFujeILvUTQmbL9JXHJd3/3FilxI.V.JWD0',
        '18800000002',
        'support@auction.local',
        'SUPPORT',
        'ACTIVE',
        '客服账号'
    ),
    (
        @demo_seller_username,
        '$6$demoSeller2026$nYmy18kRvHh/SPc.sdGkt5BZoDsv1ifIQAiK3hTNmdXIEyoAUB5RIQEGqQCkF4jeXVXAcDRmwxGFo3A3iZxmX/',
        '18800000003',
        'seller@auction.local',
        'USER',
        'ACTIVE',
        '演示卖家'
    ),
    (
        @demo_buyer_username,
        '$6$demoBuyer2026$LnOFWmHjnVMt6UGpIKbt5OAXZCERbPAlO57ePs3PfCSwUvVbJoiWb9JqNoO//qLWH994IsvcxZcNAQnmp3kMO1',
        '18800000004',
        'buyer@auction.local',
        'USER',
        'ACTIVE',
        '演示买家A'
    ),
    (
        @demo_buyer_2_username,
        '$6$demoBuyerB2026$Mill4wEWDSmsT6kDyASP1ZyE5qha/pm2BNxUxIu03erruN..zASNKrI.kVCogMsniNGRZfE87o.a4fzhwmHFd/',
        '18800000005',
        'buyer2@auction.local',
        'USER',
        'ACTIVE',
        '演示买家B'
    )
ON DUPLICATE KEY UPDATE
    password_hash = VALUES(password_hash),
    role_code = VALUES(role_code),
    status = VALUES(status),
    nickname = VALUES(nickname),
    updated_at = CURRENT_TIMESTAMP(3);

SET @admin_id := (
    SELECT user_id FROM user_account WHERE username = @demo_admin_username
);
SET @support_id := (
    SELECT user_id FROM user_account WHERE username = @demo_support_username
);
SET @seller_id := (
    SELECT user_id FROM user_account WHERE username = @demo_seller_username
);
SET @buyer_id := (
    SELECT user_id FROM user_account WHERE username = @demo_buyer_username
);
SET @buyer_2_id := (
    SELECT user_id FROM user_account WHERE username = @demo_buyer_2_username
);
SET @digital_category_id := (
    SELECT category_id FROM item_category WHERE category_name = '数码设备' LIMIT 1
);
SET @book_category_id := (
    SELECT category_id FROM item_category WHERE category_name = '图书文创' LIMIT 1
);
SET @collection_category_id := (
    SELECT category_id FROM item_category WHERE category_name = '校园收藏' LIMIT 1
);

INSERT INTO item (
    seller_id,
    category_id,
    title,
    description,
    start_price,
    item_status,
    cover_image_url
) VALUES (
    @seller_id,
    @digital_category_id,
    '演示拍品-已成交机械键盘',
    '用于答辩展示的已成交拍品，包含订单、支付、评价和统计数据。',
    200.00,
    'SOLD',
    '/uploads/demo/keyboard.jpg'
);
SET @sold_item_id := LAST_INSERT_ID();

INSERT INTO item (
    seller_id,
    category_id,
    title,
    description,
    start_price,
    item_status,
    cover_image_url
) VALUES (
    @seller_id,
    @digital_category_id,
    '演示拍品-竞拍中相机',
    '用于答辩展示实时竞价、最高价、最高出价者和延时保护。',
    120.00,
    'IN_AUCTION',
    '/uploads/demo/camera.jpg'
);
SET @running_item_id := LAST_INSERT_ID();

INSERT INTO item (
    seller_id,
    category_id,
    title,
    description,
    start_price,
    item_status,
    cover_image_url
) VALUES (
    @seller_id,
    @book_category_id,
    '演示拍品-已流拍教材',
    '用于答辩展示无人出价后的流拍状态。',
    80.00,
    'UNSOLD',
    '/uploads/demo/book.jpg'
);
SET @unsold_item_id := LAST_INSERT_ID();

INSERT INTO item (
    seller_id,
    category_id,
    title,
    description,
    start_price,
    item_status,
    cover_image_url
) VALUES (
    @seller_id,
    @collection_category_id,
    '演示拍品-待开始纪念徽章',
    '用于答辩展示待开始拍卖配置。',
    50.00,
    'READY_FOR_AUCTION',
    '/uploads/demo/badge.jpg'
);
SET @pending_item_id := LAST_INSERT_ID();

INSERT INTO item_image (item_id, image_url, sort_no, is_cover)
VALUES
    (@sold_item_id, '/uploads/demo/keyboard.jpg', 1, 1),
    (@running_item_id, '/uploads/demo/camera.jpg', 1, 1),
    (@unsold_item_id, '/uploads/demo/book.jpg', 1, 1),
    (@pending_item_id, '/uploads/demo/badge.jpg', 1, 1);

INSERT INTO item_audit_log (item_id, admin_id, audit_result, audit_comment)
VALUES
    (@sold_item_id, @admin_id, 'APPROVED', '演示数据审核通过'),
    (@running_item_id, @admin_id, 'APPROVED', '演示数据审核通过'),
    (@unsold_item_id, @admin_id, 'APPROVED', '演示数据审核通过'),
    (@pending_item_id, @admin_id, 'APPROVED', '演示数据审核通过');

INSERT INTO auction (
    item_id,
    seller_id,
    start_time,
    end_time,
    start_price,
    current_price,
    bid_step,
    highest_bidder_id,
    status,
    anti_sniping_window_seconds,
    extend_seconds,
    version
) VALUES (
    @sold_item_id,
    @seller_id,
    DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 2 DAY),
    DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 1 DAY),
    200.00,
    260.00,
    20.00,
    @buyer_id,
    'SOLD',
    60,
    60,
    3
);
SET @sold_auction_id := LAST_INSERT_ID();

INSERT INTO auction (
    item_id,
    seller_id,
    start_time,
    end_time,
    start_price,
    current_price,
    bid_step,
    highest_bidder_id,
    status,
    anti_sniping_window_seconds,
    extend_seconds,
    version
) VALUES (
    @running_item_id,
    @seller_id,
    DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 10 MINUTE),
    DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL 30 MINUTE),
    120.00,
    180.00,
    20.00,
    @buyer_2_id,
    'RUNNING',
    60,
    60,
    2
);
SET @running_auction_id := LAST_INSERT_ID();

INSERT INTO auction (
    item_id,
    seller_id,
    start_time,
    end_time,
    start_price,
    current_price,
    bid_step,
    highest_bidder_id,
    status,
    anti_sniping_window_seconds,
    extend_seconds,
    version
) VALUES (
    @unsold_item_id,
    @seller_id,
    DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 3 DAY),
    DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 2 DAY),
    80.00,
    80.00,
    10.00,
    NULL,
    'UNSOLD',
    60,
    60,
    1
);
SET @unsold_auction_id := LAST_INSERT_ID();

INSERT INTO auction (
    item_id,
    seller_id,
    start_time,
    end_time,
    start_price,
    current_price,
    bid_step,
    highest_bidder_id,
    status,
    anti_sniping_window_seconds,
    extend_seconds,
    version
) VALUES (
    @pending_item_id,
    @seller_id,
    DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL 20 MINUTE),
    DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL 80 MINUTE),
    50.00,
    50.00,
    10.00,
    NULL,
    'PENDING_START',
    60,
    60,
    0
);
SET @pending_auction_id := LAST_INSERT_ID();

INSERT INTO bid_record (
    auction_id,
    bidder_id,
    request_id,
    bid_amount,
    bid_status,
    bid_time
) VALUES
    (
        @sold_auction_id,
        @buyer_2_id,
        'DEMO-SOLD-BID-1',
        220.00,
        'OUTBID',
        DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 26 HOUR)
    ),
    (
        @sold_auction_id,
        @buyer_id,
        'DEMO-SOLD-BID-2',
        260.00,
        'WINNING',
        DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 25 HOUR)
    ),
    (
        @running_auction_id,
        @buyer_id,
        'DEMO-RUNNING-BID-1',
        160.00,
        'OUTBID',
        DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 8 MINUTE)
    ),
    (
        @running_auction_id,
        @buyer_2_id,
        'DEMO-RUNNING-BID-2',
        180.00,
        'WINNING',
        DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 5 MINUTE)
    );

INSERT INTO order_info (
    order_no,
    auction_id,
    buyer_id,
    seller_id,
    final_amount,
    order_status,
    pay_deadline_at,
    paid_at,
    shipped_at,
    completed_at
) VALUES (
    'DEMO-ORDER-PAID',
    @sold_auction_id,
    @buyer_id,
    @seller_id,
    260.00,
    'REVIEWED',
    DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 20 HOUR),
    DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 23 HOUR),
    DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 22 HOUR),
    DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 21 HOUR)
);
SET @demo_order_id := LAST_INSERT_ID();

INSERT INTO payment_record (
    payment_no,
    order_id,
    pay_channel,
    transaction_no,
    pay_amount,
    pay_status,
    callback_payload_hash,
    paid_at
) VALUES (
    'DEMO-PAYMENT-PAID',
    @demo_order_id,
    'MOCK_ALIPAY',
    'DEMO-TXN-PAID',
    260.00,
    'SUCCESS',
    'demo-payment-callback-hash',
    DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 23 HOUR)
);
SET @demo_payment_id := LAST_INSERT_ID();

INSERT INTO payment_callback_log (
    payment_id,
    order_id,
    transaction_no,
    callback_status,
    raw_payload,
    processed_at,
    process_result
) VALUES (
    @demo_payment_id,
    @demo_order_id,
    'DEMO-TXN-PAID',
    'PROCESSED',
    '{"demo":true,"payStatus":"SUCCESS"}',
    DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 23 HOUR),
    'payment success applied'
);

INSERT INTO review (
    order_id,
    reviewer_id,
    reviewee_id,
    review_type,
    rating,
    content
) VALUES
    (
        @demo_order_id,
        @buyer_id,
        @seller_id,
        'BUYER_TO_SELLER',
        5,
        '发货及时，拍品与描述一致。'
    ),
    (
        @demo_order_id,
        @seller_id,
        @buyer_id,
        'SELLER_TO_BUYER',
        5,
        '付款及时，沟通顺畅。'
    );

INSERT INTO notification (
    user_id,
    notice_type,
    title,
    content,
    biz_type,
    biz_id,
    read_status,
    push_status
) VALUES
    (
        @buyer_id,
        'PAYMENT_SUCCESS',
        '演示订单支付成功',
        '演示订单 DEMO-ORDER-PAID 已支付成功。',
        'ORDER',
        @demo_order_id,
        'UNREAD',
        'SENT'
    ),
    (
        @seller_id,
        'ORDER_PAID',
        '演示买家已付款',
        '演示订单 DEMO-ORDER-PAID 已完成付款。',
        'ORDER',
        @demo_order_id,
        'UNREAD',
        'SENT'
    ),
    (
        @buyer_id,
        'OUTBID',
        '演示出价被超越',
        '演示拍品-竞拍中相机 当前最高价为 180.00。',
        'AUCTION',
        @running_auction_id,
        'UNREAD',
        'SENT'
    );

INSERT INTO task_log (
    task_type,
    biz_key,
    task_status,
    retry_count,
    last_error,
    scheduled_at,
    started_at,
    finished_at
) VALUES
    (
        'AUCTION_FINISH',
        'DEMO-AUCTION-SOLD',
        'SUCCESS',
        0,
        '',
        DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 1 DAY),
        DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 1 DAY),
        DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 1 DAY)
    ),
    (
        'ORDER_SETTLEMENT',
        'DEMO-ORDER-PAID',
        'SUCCESS',
        0,
        '',
        DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 23 HOUR),
        DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 23 HOUR),
        DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 23 HOUR)
    );

INSERT INTO operation_log (
    operator_id,
    operator_role,
    module_name,
    operation_name,
    biz_key,
    result,
    detail
) VALUES
    (
        @admin_id,
        'ADMIN',
        'demo',
        'prepare_demo_data',
        'DEMO-ORDER-PAID',
        'SUCCESS',
        'S15 demo data prepared'
    ),
    (
        @support_id,
        'SUPPORT',
        'ops',
        'view_demo_logs',
        'DEMO-ORDER-PAID',
        'SUCCESS',
        'S15 demo support account can inspect logs'
    );

INSERT INTO statistics_daily (
    stat_date,
    auction_count,
    sold_count,
    unsold_count,
    bid_count,
    gmv_amount
) VALUES (
    CURRENT_DATE,
    4,
    1,
    1,
    4,
    260.00
)
ON DUPLICATE KEY UPDATE
    auction_count = VALUES(auction_count),
    sold_count = VALUES(sold_count),
    unsold_count = VALUES(unsold_count),
    bid_count = VALUES(bid_count),
    gmv_amount = VALUES(gmv_amount),
    updated_at = CURRENT_TIMESTAMP(3);
