"use client";

import { useCallback, useEffect, useRef, useState } from "react";

export type AuctionEventMessage = {
  event: string;
  auction_id: number;
  current_price: number;
  highest_bidder_masked: string;
  end_time: string;
  server_time: string;
};

type UseAuctionWebSocketOptions = {
  auctionId: string;
  enabled?: boolean;
  onPriceUpdate?: (message: AuctionEventMessage) => void;
  onAuctionExtended?: (message: AuctionEventMessage) => void;
  onOutbid?: (message: AuctionEventMessage) => void;
};

type ConnectionState = "connecting" | "connected" | "disconnected" | "fallback";

const RECONNECT_DELAY = 3000;
const MAX_RECONNECT_ATTEMPTS = 3;
const PING_INTERVAL = 30000;

export function useAuctionWebSocket({
  auctionId,
  enabled = true,
  onPriceUpdate,
  onAuctionExtended,
  onOutbid,
}: UseAuctionWebSocketOptions) {
  const [state, setState] = useState<ConnectionState>("disconnected");
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectAttemptsRef = useRef(0);
  const reconnectTimerRef = useRef<NodeJS.Timeout | null>(null);
  const pingTimerRef = useRef<NodeJS.Timeout | null>(null);
  const intentionallyClosedRef = useRef(false);

  // Store latest callbacks in refs to avoid reconnect storms from unstable references
  const callbacksRef = useRef({ onPriceUpdate, onAuctionExtended, onOutbid });
  callbacksRef.current = { onPriceUpdate, onAuctionExtended, onOutbid };

  const cleanup = useCallback(() => {
    intentionallyClosedRef.current = true;
    if (pingTimerRef.current) {
      clearInterval(pingTimerRef.current);
      pingTimerRef.current = null;
    }
    if (reconnectTimerRef.current) {
      clearTimeout(reconnectTimerRef.current);
      reconnectTimerRef.current = null;
    }
    if (wsRef.current) {
      wsRef.current.close();
      wsRef.current = null;
    }
  }, []);

  const connect = useCallback(() => {
    if (!enabled || !auctionId) {
      return;
    }

    intentionallyClosedRef.current = false;
    cleanup();

    const wsBaseUrl =
      process.env.NEXT_PUBLIC_WS_BASE_URL || "ws://127.0.0.1:18080";
    const url = `${wsBaseUrl}/ws/auction/${auctionId}`;

    setState("connecting");

    try {
      const ws = new WebSocket(url);
      wsRef.current = ws;

      ws.onopen = () => {
        if (wsRef.current !== ws) {
          return;
        }
        setState("connected");
        reconnectAttemptsRef.current = 0;

        const token = localStorage.getItem("auction_token");
        if (token) {
          ws.send(
            JSON.stringify({
              action: "identify",
              token,
            })
          );
        }

        pingTimerRef.current = setInterval(() => {
          if (ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({ action: "ping" }));
          }
        }, PING_INTERVAL);
      };

      ws.onmessage = (event) => {
        try {
          const message: AuctionEventMessage = JSON.parse(event.data);

          if (message.event === "PONG" || message.event === "CONNECTED") {
            return;
          }

          // Read callbacks from ref to avoid stale closures
          const { onPriceUpdate, onAuctionExtended, onOutbid } = callbacksRef.current;

          switch (message.event) {
            case "PRICE_UPDATED":
              onPriceUpdate?.(message);
              break;
            case "AUCTION_EXTENDED":
              onAuctionExtended?.(message);
              break;
            case "OUTBID":
              onOutbid?.(message);
              break;
          }
        } catch {
          // Ignore malformed messages
        }
      };

      ws.onclose = () => {
        // Skip if this close was intentional (cleanup() or a new connection replaced this one)
        if (intentionallyClosedRef.current || wsRef.current !== ws) {
          return;
        }

        if (pingTimerRef.current) {
          clearInterval(pingTimerRef.current);
          pingTimerRef.current = null;
        }

        if (reconnectAttemptsRef.current < MAX_RECONNECT_ATTEMPTS) {
          setState("disconnected");
          reconnectTimerRef.current = setTimeout(() => {
            reconnectAttemptsRef.current++;
            connect();
          }, RECONNECT_DELAY);
        } else {
          setState("fallback");
        }
      };

      ws.onerror = () => {
        ws.close();
      };
    } catch {
      setState("fallback");
    }
    // connect depends on enabled and auctionId only; callbacks are read from ref
  }, [enabled, auctionId, cleanup]);

  useEffect(() => {
    if (enabled) {
      connect();
    } else {
      cleanup();
      setState("disconnected");
    }

    return cleanup;
  }, [enabled, connect, cleanup]);

  const reconnect = useCallback(() => {
    reconnectAttemptsRef.current = 0;
    connect();
  }, [connect]);

  return {
    state,
    isConnected: state === "connected",
    isFallback: state === "fallback",
    reconnect,
  };
}
