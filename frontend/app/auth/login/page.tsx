"use client";

export const dynamic = "force-dynamic";

import { FormEvent, useState } from "react";
import { useRouter } from "next/navigation";
import { Gavel, Loader2, ShieldCheck } from "lucide-react";
import { login } from "@/lib/api/client";
import { useAuth } from "@/lib/auth-context";
import { Button } from "@/components/ui/button";
import { Toast } from "@/components/ui/toast";

export default function LoginPage() {
  const router = useRouter();
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [loading, setLoading] = useState(false);
  const [toast, setToast] = useState("");
  const { refresh } = useAuth();

  async function handleSubmit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    setLoading(true);
    try {
      const result = await login(username, password);
      await refresh();
      setToast(`登录成功，欢迎 ${result.user_info.nickname}`);
      setTimeout(() => router.push("/auction/hall"), 650);
    } catch (e) {
      setToast(e instanceof Error ? e.message : "登录失败");
      setLoading(false);
    }
  }

  return (
    <main className="grid min-h-screen bg-slate-50 lg:grid-cols-[45%_55%]">
      <Toast message={toast} />
      <section className="relative hidden overflow-hidden border-r border-slate-200 bg-slate-100 p-10 lg:flex lg:flex-col lg:justify-between">
        <div className="absolute inset-0 bg-[linear-gradient(135deg,rgba(79,70,229,0.16),transparent_45%),linear-gradient(315deg,rgba(16,185,129,0.16),transparent_40%)]" />
        <div className="relative flex items-center gap-3 text-lg font-black text-slate-950">
          <span className="flex h-11 w-11 items-center justify-center rounded-lg bg-indigo-600 text-white">
            <Gavel className="h-5 w-5" />
          </span>
          校园拍卖
        </div>
        <div className="relative max-w-md">
          <p className="text-sm font-bold text-indigo-700">TRUSTED CAMPUS EXCHANGE</p>
          <h1 className="mt-4 text-4xl font-black leading-tight text-slate-950">把每一次出价都放进可验证的交易轨道</h1>
          <p className="mt-5 leading-8 text-slate-600">登录后可直接进入真实拍卖大厅，所有关键状态以服务端权威结果为准。</p>
        </div>
      </section>
      <section className="flex items-center justify-center px-4 py-12">
        <form onSubmit={handleSubmit} className="w-full max-w-md rounded-lg border border-slate-200 bg-white p-8 shadow-float">
          <div className="mb-8">
            <div className="mb-4 flex h-12 w-12 items-center justify-center rounded-lg bg-indigo-50 text-indigo-600">
              <ShieldCheck className="h-6 w-6" />
            </div>
            <h2 className="text-3xl font-black text-slate-950">登录 / 注册</h2>
            <p className="mt-2 text-sm leading-6 text-slate-600">使用真实账号进入正式业务工作台。</p>
          </div>
          <label className="mb-2 block text-sm font-bold text-slate-700">用户名</label>
          <input value={username} onChange={(event) => setUsername(event.target.value)} className="mb-5 h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 text-sm outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-500/20" />
          <label className="mb-2 block text-sm font-bold text-slate-700">密码</label>
          <input value={password} type="password" onChange={(event) => setPassword(event.target.value)} className="mb-6 h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 text-sm outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-500/20" />
          <Button className="w-full" disabled={loading || !username || !password}>
            {loading ? <Loader2 className="h-4 w-4 animate-spin" /> : null}
            {loading ? "登录中" : "进入拍卖大厅"}
          </Button>
        </form>
      </section>
    </main>
  );
}
