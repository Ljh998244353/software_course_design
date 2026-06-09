"use client";

import { useQuery } from "@tanstack/react-query";
import { AnimatePresence, motion } from "framer-motion";
import { BellRing, X } from "lucide-react";
import { useEffect, useMemo, useState } from "react";
import { getNotifications } from "@/lib/api/client";
import { useAuth } from "@/lib/auth-context";
import { queryKeys } from "@/lib/query/keys";
import type { UserNotification } from "@/types/auction";

const seenNotificationKey = "last_seen_notification_id";

export function NotificationLiveRegion() {
  const { user, loading } = useAuth();
  const [visibleNotice, setVisibleNotice] = useState<UserNotification | null>(null);

  const query = useQuery({
    queryKey: queryKeys.notifications(true),
    queryFn: () => getNotifications(true),
    enabled: Boolean(user) && !loading,
    refetchInterval: 5000,
  });

  const latestUnread = useMemo(() => {
    const list = query.data?.list ?? [];
    return list
      .filter((notice) => notice.readStatus === "UNREAD")
      .sort((left, right) => Number(right.notificationId) - Number(left.notificationId))[0] ?? null;
  }, [query.data?.list]);

  useEffect(() => {
    if (!latestUnread || typeof window === "undefined") {
      return;
    }
    const lastSeenId = Number(sessionStorage.getItem(seenNotificationKey) ?? "0");
    if (latestUnread.notificationId <= lastSeenId) {
      return;
    }
    sessionStorage.setItem(seenNotificationKey, String(latestUnread.notificationId));
    setVisibleNotice(latestUnread);
  }, [latestUnread]);

  useEffect(() => {
    if (!visibleNotice) {
      return;
    }
    const timer = window.setTimeout(() => setVisibleNotice(null), 6500);
    return () => window.clearTimeout(timer);
  }, [visibleNotice]);

  return (
    <AnimatePresence>
      {visibleNotice ? (
        <motion.aside
          initial={{ opacity: 0, y: 18, scale: 0.98 }}
          animate={{ opacity: 1, y: 0, scale: 1 }}
          exit={{ opacity: 0, y: 18, scale: 0.98 }}
          className="fixed bottom-6 right-6 z-50 w-[min(360px,calc(100vw-32px))] rounded-lg border border-indigo-100 bg-white/95 p-4 shadow-modal backdrop-blur"
          role="status"
          aria-live="polite"
        >
          <div className="flex items-start gap-3">
            <span className="flex h-10 w-10 shrink-0 items-center justify-center rounded-lg bg-indigo-50 text-indigo-600">
              <BellRing className="h-5 w-5" />
            </span>
            <div className="min-w-0 flex-1">
              <p className="text-sm font-black text-slate-950">{visibleNotice.title}</p>
              <p className="mt-1 max-h-10 overflow-hidden text-sm leading-5 text-slate-600">{visibleNotice.content}</p>
            </div>
            <button
              onClick={() => setVisibleNotice(null)}
              className="rounded-lg p-1 text-slate-400 hover:bg-slate-100 hover:text-slate-700"
              aria-label="关闭通知"
            >
              <X className="h-4 w-4" />
            </button>
          </div>
        </motion.aside>
      ) : null}
    </AnimatePresence>
  );
}
