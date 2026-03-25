-- atomic_bid.lua
-- 键: auction:{item_id}  字段: price, bidder, end_time, step
-- 参数: KEYS[1]=item_key, ARGV[1]=amount, ARGV[2]=bidder_id, ARGV[3]=timestamp

local current_price = tonumber(redis.call('HGET', KEYS[1], 'price'))
local bid_step = tonumber(redis.call('HGET', KEYS[1], 'step'))
local end_time = tonumber(redis.call('HGET', KEYS[1], 'end_time'))
local current_bidder = redis.call('HGET', KEYS[1], 'bidder')
local anti_snipe_N = tonumber(ARGV[4])
local anti_snipe_M = tonumber(ARGV[5])

-- 校验出价金额
if tonumber(ARGV[1]) < current_price + bid_step then
    return {'err', 'BID_TOO_LOW', current_price}
end

-- 校验拍卖是否结束
if tonumber(ARGV[3]) > end_time then
    return {'err', 'AUCTION_ENDED'}
end

-- 检查是否触发 anti-snipe (结束前 N 秒内)
local remaining = end_time - tonumber(ARGV[3])
if remaining < anti_snipe_N then
    local new_end_time = end_time + anti_snipe_M
    redis.call('HSET', KEYS[1], 'price', ARGV[1])
    redis.call('HSET', KEYS[1], 'bidder', ARGV[2])
    redis.call('HSET', KEYS[1], 'end_time', new_end_time)
    return {'ok', 'BID_ACCEPTED_EXTENDED', ARGV[1], new_end_time}
end

-- 更新最高价
redis.call('HSET', KEYS[1], 'price', ARGV[1])
redis.call('HSET', KEYS[1], 'bidder', ARGV[2])

return {'ok', 'BID_ACCEPTED', ARGV[1]}
