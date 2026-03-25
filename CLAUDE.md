# CLAUDE.md

该项目所有文档用中文,回答用中文!
不要尝试提权!!!
不要尝试提权!!!
不要尝试提权!!!


This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a course design project for an **Online Auction System** built with high-performance C++ architecture. The README.md contains the full requirements specification.

## Planned Technology Stack

- **Language:** C++20 (Coroutines, Concepts, RAII, smart pointers)
- **Backend Framework:** Drogon or Workflow (non-blocking I/O, epoll-based)
- **Database:** MySQL 8.0 (persistent storage for users, items, orders, bids)
- **Cache:** Redis (hot data,竞价 concurrency control, timing wheels)
- **Build System:** CMake 3.20+

## Architecture

The system follows a layered architecture:

1. **Access Layer** - Drogon/Epoll non-blocking network stack for HTTP/WebSocket
2. **Core Logic Layer** - In-memory matching engine for bid processing, Workflow task-based async services
3. **Persistence Layer** - Redis for real-time竞价 data, MySQL for durable storage

## Key Design Patterns

- **Hierarchical Timing Wheel** - O(1) timer management for auction countdowns (replaces per-auction hardware timers)
- **Lock-free Double Buffering** - Async log persistence to avoid blocking the bid engine
- **Multi-threaded Reactor** - Main reactor accepts connections, Sub-reactors handle I/O, worker pool handles compute
- **Atomic Bidding** - Redis Lua scripts for atomic price updates, optimistic locking with version fields in MySQL

## Database Schema (from README)

Core tables: `users`, `items`, `auctions`, `auction_items`, `bids`, `orders`, `payments`, `comments`

Key indexes:
- `bids(auction_item_id, amount, created_at)` composite index
- `items(status)` enum index
- `auction_items(item_id)` with clustering

## Project Status

This is a requirements document only - no source code has been implemented yet. The first commit is the README.md specification.
