"use client";

export const dynamic = "force-dynamic";

import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { Bell, CheckCheck, Loader2 } from "lucide-react";
import { useState } from "react";
import { AuthGuard } from "@/components/layout/auth-guard";
import { SiteNav } from "@/components/layout/site-nav";
import { Button } from "@/components/ui/button";
import { Skeleton } from "@/components/ui/skeleton";
import { Toast } from "@/components/ui/toast";
import { getNotifications, markNotificationRead } from "@/lib/api/client";
import { queryKeys } from "@/lib/query/keys";
import { formatTime } from "@/lib/utils/format";

export default function NotificationsPage() {
  return (
    <AuthGuard requireAuth>
      <NotificationsContent />
    </AuthGuard>
  );
}

function NotificationsContent() {
  const [unreadOnly, setUnreadOnly] = useState(false);
  const queryClient = useQueryClient();
  const query = useQuery({
    queryKey: queryKeys.notifications(unreadOnly),
    queryFn: () => getNotifications(unreadOnly),
  });
  const mutation = useMutation({
    mutationFn: markNotificationRead,
    onSuccess: () => {
      void queryClient.invalidateQueries({ queryKey: queryKeys.notifications(false) });
      void queryClient.invalidateQueries({ queryKey: queryKeys.notifications(true) });
    },
  });
  const notifications = query.data?.list ?? [];

  return (
    <>
      <SiteNav />
      <Toast message={mutation.isSuccess ? "已标记为已读" : mutation.error?.message ?? ""} tone={mutation.isError ? "danger" : "success"} />
      <main className="mx-auto max-w-4xl px-4 py-8 sm:px-6 lg:px-8">
        <div className="mb-6 flex flex-wrap items-end justify-between gap-4">
          <div>
            <p className="text-sm font-bold text-indigo-600">NOTIFICATIONS</p>
            <h1 className="mt-1 text-3xl font-black text-slate-950">站内通知</h1>
          </div>
          <Button variant={unreadOnly ? "primary" : "secondary"} onClick={() => setUnreadOnly((value) => !value)}>
            <Bell className="h-4 w-4" />
            未读 {query.data?.unreadCount ?? 0}
          </Button>
        </div>

        {query.isLoading ? (
          <Skeleton className="h-72" />
        ) : notifications.length === 0 ? (
          <div className="rounded-lg border border-slate-200 bg-white px-6 py-12 text-center shadow-card">
            <Bell className="mx-auto mb-4 h-9 w-9 text-slate-400" />
            <p className="font-bold text-slate-700">暂无通知</p>
          </div>
        ) : (
          <div className="space-y-3">
            {notifications.map((notice) => (
              <article key={notice.notificationId} className="rounded-lg border border-slate-200 bg-white p-4 shadow-card">
                <div className="flex flex-wrap items-start justify-between gap-3">
                  <div>
                    <div className="flex flex-wrap items-center gap-2">
                      <h2 className="font-black text-slate-950">{notice.title}</h2>
                      <span className={`rounded-full px-2.5 py-1 text-xs font-black ${notice.readStatus === "UNREAD" ? "bg-amber-50 text-amber-700" : "bg-slate-100 text-slate-500"}`}>
                        {notice.readStatus === "UNREAD" ? "未读" : "已读"}
                      </span>
                    </div>
                    <p className="mt-2 text-sm leading-6 text-slate-600">{notice.content}</p>
                    <p className="mt-2 text-xs text-slate-400">{formatTime(notice.createdAt)}</p>
                  </div>
                  <Button
                    variant="secondary"
                    disabled={notice.readStatus === "READ" || mutation.isPending}
                    onClick={() => mutation.mutate(notice.notificationId)}
                  >
                    {mutation.isPending ? <Loader2 className="h-4 w-4 animate-spin" /> : <CheckCheck className="h-4 w-4" />}
                    已读
                  </Button>
                </div>
              </article>
            ))}
          </div>
        )}
      </main>
    </>
  );
}
