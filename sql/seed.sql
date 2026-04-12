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
        'admin',
        '$6$GiXyqr1v6ijjKP77$AnxQInLYg15SsHGvrvGd5QUYMUgduCV9JcpTfUOeNSpqdOQjrDb/IafGu6IOTQT81ngky7Kt0.GiIi7UzdIkM1',
        '18800000001',
        'admin@auction.local',
        'ADMIN',
        'ACTIVE',
        '平台管理员'
    ),
    (
        'support',
        '$6$8vWayW6dSqTju8au$qRheHHOCQt4IovFff4arFfmBxZtlZDeS9I9xqR.eQ/w/rXJqtmOnujY9BnpCyEyDFkHsJ41LZO2/4J5tJFJWw/',
        '18800000002',
        'support@auction.local',
        'SUPPORT',
        'ACTIVE',
        '客服账号'
    ),
    (
        'seller_demo',
        '$6$o5Fe9sOKNX32K6g7$ujImHbAzLsIulIq8PQoGD/02wvpm/Vc3eFxdXSuXQDosC8TQJmVhJAnJydXdGQ5pQ3kMtduYLdtiTc6akTdro1',
        '18800000003',
        'seller@auction.local',
        'USER',
        'ACTIVE',
        '演示卖家'
    ),
    (
        'buyer_demo',
        '$6$TmU5LHJ4VPBnqHL1$ugYDXtGPzeZf2RruDZYHT1xdiTdibAtyJLKBJPP7OCnZwjOEAIAgEqNIltS833loAHo0uOTpzQGxXDHbE8LOa0',
        '18800000004',
        'buyer@auction.local',
        'USER',
        'ACTIVE',
        '演示买家'
    )
ON DUPLICATE KEY UPDATE
    password_hash = VALUES(password_hash),
    role_code = VALUES(role_code),
    status = VALUES(status),
    nickname = VALUES(nickname),
    updated_at = CURRENT_TIMESTAMP(3);

INSERT INTO item_category (
    category_name,
    status,
    sort_no
) VALUES
    ('数码设备', 'ACTIVE', 10),
    ('图书文创', 'ACTIVE', 20),
    ('运动户外', 'ACTIVE', 30),
    ('校园收藏', 'ACTIVE', 40)
ON DUPLICATE KEY UPDATE
    status = VALUES(status),
    sort_no = VALUES(sort_no),
    updated_at = CURRENT_TIMESTAMP(3);
