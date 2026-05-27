"use client";

import { ChangeEvent, useEffect, useState } from "react";
import type { ReactNode } from "react";
import { CheckCircle2, ImagePlus, Loader2, Save, Send, UploadCloud } from "lucide-react";
import { SiteNav } from "@/components/layout/site-nav";
import { Button } from "@/components/ui/button";
import { Toast } from "@/components/ui/toast";
import { addItemImage, createItem, submitItemForReview } from "@/lib/api/client";

const steps = ["描述拍品", "上传实拍", "制定竞价契约", "检查并上架"];
const draftStorageKey = "auction-publish-draft";

type PublishDraft = {
  title?: string;
  description?: string;
  price?: string;
  images?: string[];
  createdItemId?: string;
  addedImageUrls?: string[];
};

export default function PublishPage() {
  const [step, setStep] = useState(0);
  const [title, setTitle] = useState("");
  const [description, setDescription] = useState("");
  const [price, setPrice] = useState("300");
  const [images, setImages] = useState<string[]>([]);
  const [toast, setToast] = useState("");
  const [toastTone, setToastTone] = useState<"success" | "danger">("success");
  const [idempotencyKey, setIdempotencyKey] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [createdItemId, setCreatedItemId] = useState<string | null>(null);
  const [addedImageUrls, setAddedImageUrls] = useState<string[]>([]);
  const [submittedItemId, setSubmittedItemId] = useState<string | null>(null);

  useEffect(() => {
    setIdempotencyKey(crypto.randomUUID());
    const saved = localStorage.getItem(draftStorageKey);
    if (saved) {
      const draft = JSON.parse(saved) as PublishDraft;
      setTitle(draft.title ?? "");
      setDescription(draft.description ?? "");
      setPrice(draft.price ?? "300");
      setImages(draft.images ?? []);
      setCreatedItemId(draft.createdItemId ?? null);
      setAddedImageUrls(draft.addedImageUrls ?? []);
    }
  }, []);

  function saveDraft(next?: Partial<PublishDraft>) {
    const payload = { title, description, price, images, createdItemId, addedImageUrls, ...next };
    localStorage.setItem(draftStorageKey, JSON.stringify(payload));
    setToast("草稿已自动保存");
  }

  function rememberCreatedItem(itemId: string) {
    setCreatedItemId(itemId);
    saveDraft({ createdItemId: itemId });
  }

  function addImage(event: ChangeEvent<HTMLInputElement>) {
    const value = event.target.value;
    if (!value) {
      return;
    }
    const next = [...images, value];
    setImages(next);
    saveDraft({ images: next });
  }

  async function submit() {
    if (submitting) return;
    setSubmitting(true);
    setToast("");
    try {
      if (images.length === 0) {
        throw new Error("请至少填写一张图片 URL 后再提交审核");
      }

      let itemId = createdItemId;
      if (!itemId) {
        const created = await createItem({
          title,
          description,
          category_id: 1,
          start_price: parseFloat(price) || 0,
          cover_image_url: images[0] ?? "",
        });
        itemId = String(created.item_id);
        rememberCreatedItem(itemId);
      }

      let nextAddedImageUrls = [...addedImageUrls];
      for (const [index, image] of images.entries()) {
        if (nextAddedImageUrls.includes(image)) {
          continue;
        }
        await addItemImage(itemId, {
          image_url: image,
          sort_no: index + 1,
          is_cover: index === 0,
        });
        nextAddedImageUrls = [...nextAddedImageUrls, image];
        setAddedImageUrls(nextAddedImageUrls);
        saveDraft({ createdItemId: itemId, addedImageUrls: nextAddedImageUrls });
      }

      const reviewed = await submitItemForReview(itemId);
      setSubmittedItemId(String(reviewed.item_id));
      setToast(`提交成功，拍品 #${reviewed.item_id} 已进入审核队列`);
      setToastTone("success");
      localStorage.removeItem(draftStorageKey);
      setCreatedItemId(null);
      setAddedImageUrls([]);
    } catch (err) {
      const message = err instanceof Error ? err.message : "提交失败";
      setToast(message);
      setToastTone("danger");
    } finally {
      setSubmitting(false);
    }
  }

  return (
    <>
      <SiteNav />
      <Toast message={toast} tone={toastTone} />
      <main className="mx-auto max-w-6xl px-4 py-8 sm:px-6 lg:px-8">
        <div className="mb-6 rounded-lg border border-slate-200 bg-white p-6 shadow-card">
          <p className="text-sm font-bold text-indigo-600">PUBLISH WIZARD</p>
          <h1 className="mt-1 text-3xl font-black text-slate-950">拍品发布</h1>
          <p className="mt-2 text-sm leading-6 text-slate-600">首轮使用图片 URL 提交，系统会保存草稿、图片元数据并进入审核队列。</p>
        </div>

        <div className="grid gap-6 lg:grid-cols-[280px_1fr]">
          <aside className="h-fit rounded-lg border border-slate-200 bg-white p-4 shadow-card">
            {steps.map((item, index) => (
              <button key={item} onClick={() => setStep(index)} className={`mb-2 flex w-full items-center gap-3 rounded-lg px-3 py-3 text-left text-sm font-bold ${step === index ? "bg-indigo-50 text-indigo-700" : "text-slate-600 hover:bg-slate-50"}`}>
                <span className="tabular flex h-7 w-7 items-center justify-center rounded-full border border-current text-xs">0{index + 1}</span>
                {item}
              </button>
            ))}
            <div className="mt-5 rounded-lg bg-slate-50 p-3 text-xs leading-5 text-slate-500">
              幂等 Token
              <p className="tabular mt-1 break-all font-bold text-slate-700">{idempotencyKey}</p>
            </div>
          </aside>

          <section className="rounded-lg border border-slate-200 bg-white p-6 shadow-card">
            {step === 0 ? (
              <div className="space-y-5">
                <Field label="拍品标题">
                  <input value={title} onBlur={() => saveDraft()} onChange={(event) => setTitle(event.target.value)} className="h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-500/20" placeholder="例如：iPad Air 5 64G 蓝色" />
                </Field>
                <Field label="拍品描述">
                  <textarea value={description} onBlur={() => saveDraft()} onChange={(event) => setDescription(event.target.value)} className="min-h-36 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 py-3 outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-500/20" placeholder="描述成色、配件、交易方式和注意事项" />
                </Field>
              </div>
            ) : null}

            {step === 1 ? (
              <div>
                <div className="rounded-lg border-2 border-dashed border-slate-300 bg-slate-50 p-8 text-center hover:border-indigo-400 hover:bg-indigo-50/40">
                  <UploadCloud className="mx-auto mb-3 h-10 w-10 text-indigo-600" />
                  <p className="font-black text-slate-950">粘贴图片 URL 作为首轮占位上传</p>
                  <input onBlur={() => saveDraft()} onChange={addImage} className="mx-auto mt-5 h-11 w-full max-w-xl rounded-lg border border-slate-200 bg-white px-3 text-sm outline-none focus:border-indigo-500" placeholder="https://images.unsplash.com/..." />
                </div>
                <div className="mt-5 grid gap-3 sm:grid-cols-4">
                  {images.map((image, index) => (
                    <div key={image} className="relative aspect-square rounded-lg border border-slate-200 bg-cover bg-center" style={{ backgroundImage: `url(${image})` }}>
                      {index === 0 ? <span className="absolute left-2 top-2 rounded-full bg-white/90 px-2 py-1 text-xs font-bold text-indigo-700">封面首图</span> : null}
                    </div>
                  ))}
                  {images.length === 0 ? (
                    <div className="flex aspect-square items-center justify-center rounded-lg border border-slate-200 bg-slate-50 text-slate-400">
                      <ImagePlus className="h-8 w-8" />
                    </div>
                  ) : null}
                </div>
              </div>
            ) : null}

            {step === 2 ? (
              <div className="grid gap-5 sm:grid-cols-2">
                <Field label="起拍价">
                  <input value={price} onBlur={() => saveDraft()} onChange={(event) => setPrice(event.target.value)} className="tabular h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500" />
                </Field>
                <Field label="最小加价幅度">
                  <input className="tabular h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500" defaultValue="20" />
                </Field>
                <Field label="延时保护">
                  <select className="h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500">
                    <option>最后 5 分钟内出价顺延 3 分钟</option>
                  </select>
                </Field>
                <Field label="交易方式">
                  <select className="h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500">
                    <option>校园当面交易</option>
                    <option>自提</option>
                  </select>
                </Field>
              </div>
            ) : null}

            {step === 3 ? (
              <div className="space-y-4">
                {[
                  ["标题", title || "未填写"],
                  ["描述", description || "未填写"],
                  ["起拍价", `¥${price}`],
                  ["图片数量", `${images.length} 张`]
                ].map(([label, value]) => (
                  <div key={label} className="flex items-center justify-between rounded-lg border border-slate-200 bg-slate-50 px-4 py-3 text-sm">
                    <span className="font-bold text-slate-600">{label}</span>
                    <span className="font-black text-slate-950">{value}</span>
                  </div>
                ))}
                <div className="flex items-center gap-2 rounded-lg border border-emerald-200 bg-emerald-50 px-4 py-3 text-sm font-bold text-emerald-700">
                  <CheckCircle2 className="h-5 w-5" />
                  {submittedItemId
                    ? `拍品 #${submittedItemId} 已提交审核`
                    : "提交后进入管理员审核队列"}
                </div>
              </div>
            ) : null}

            <div className="mt-8 flex justify-between border-t border-slate-100 pt-5">
              <Button variant="secondary" onClick={() => setStep(Math.max(step - 1, 0))}>上一步</Button>
              {step < steps.length - 1 ? (
                <Button onClick={() => setStep(Math.min(step + 1, steps.length - 1))}>下一步</Button>
              ) : (
                <Button onClick={submit} disabled={submitting}>
                  {submitting ? <Loader2 className="h-4 w-4 animate-spin" /> : <Send className="h-4 w-4" />}
                  {submitting ? "提交中..." : "提交审核"}
                </Button>
              )}
            </div>
            <button onClick={() => saveDraft()} className="mt-4 inline-flex items-center gap-2 text-sm font-bold text-slate-500 hover:text-indigo-700">
              <Save className="h-4 w-4" />
              手动保存草稿
            </button>
          </section>
        </div>
      </main>
    </>
  );
}

function Field({ label, children }: { label: string; children: ReactNode }) {
  return (
    <label className="block">
      <span className="mb-2 block text-sm font-bold text-slate-700">{label}</span>
      {children}
    </label>
  );
}
