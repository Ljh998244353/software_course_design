"use client";

import { useRouter } from "next/navigation";
import { useEffect } from "react";
import { useAuth } from "@/lib/auth-context";
import { Loader2 } from "lucide-react";

interface AuthGuardProps {
  children: React.ReactNode;
  requireAuth?: boolean;
  requireAdmin?: boolean;
  fallbackPath?: string;
}

export function AuthGuard({
  children,
  requireAuth = false,
  requireAdmin = false,
  fallbackPath = "/auth/login",
}: AuthGuardProps) {
  const { user, loading } = useAuth();
  const router = useRouter();
  const shouldBlock = requireAuth || requireAdmin;

  useEffect(() => {
    if (loading) return;

    if (requireAuth && !user) {
      router.replace(fallbackPath);
      return;
    }

    if (requireAdmin && user?.role_code !== "ADMIN") {
      router.replace("/");
      return;
    }
  }, [user, loading, requireAuth, requireAdmin, fallbackPath, router]);

  if (loading && shouldBlock) {
    return (
      <div className="flex min-h-screen items-center justify-center">
        <div className="flex flex-col items-center gap-4">
          <Loader2 className="h-8 w-8 animate-spin text-indigo-600" />
          <p className="text-sm text-slate-500">验证权限中...</p>
        </div>
      </div>
    );
  }

  if (requireAuth && !user) {
    return null;
  }

  if (requireAdmin && user?.role_code !== "ADMIN") {
    return null;
  }

  return <>{children}</>;
}
