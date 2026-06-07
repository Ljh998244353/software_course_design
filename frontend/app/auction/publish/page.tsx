"use client";

export const dynamic = "force-dynamic";

import { ChangeEvent, useEffect, useState } from "react";
import type { ReactNode } from "react";
import { CheckCircle2, ImagePlus, Loader2, Save, Send, UploadCloud, X } from "lucide-react";
import { AuthGuard } from "@/components/layout/auth-guard";
import { SiteNav } from "@/components/layout/site-nav";
import { Button } from "@/components/ui/button";
import { Toast } from "@/components/ui/toast";
import { addItemImage, createItem, submitItemForReview } from "@/lib/api/client";

const steps = ["描述拍品", "上传实拍", "建议竞价配置", "检查并提交审核"];
const draftStorageKey = "auction-publish-draft";

type PublishDraft = {
  title?: string;
  description?: string;
  price?: string;
  categoryId?: string;
  tradeMode?: string;
  location?: string;
  tags?: string;
  suggestedBidStep?: string;
  antiSnipingWindow?: string;
  extendSeconds?: string;
  images?: string[];
  createdItemId?: string;
  addedImageUrls?: string[];
};

function dedupeImageUrls(urls: string[]): string[] {
  const seen = new Set<string>();
  const result: string[] = [];
  for (const url of urls) {
    const normalized = url.trim();
    if (!normalized || seen.has(normalized)) continue;
    seen.add(normalized);
    result.push(normalized);
  }
  return result;
}

const allowedImageExtensions = new Set(["png", "jpg", "jpeg", "webp", "gif", "bmp", "svg", "avif"]);

function isLikelyImageUrl(url: URL) {
  const pathname = url.pathname.toLowerCase();
  const ext = pathname.includes(".") ? pathname.slice(pathname.lastIndexOf(".") + 1) : "";
  if (ext && allowedImageExtensions.has(ext)) {
    return true;
  }

  const format = url.searchParams.get("format")?.toLowerCase();
  const fm = url.searchParams.get("fm")?.toLowerCase();
  return Boolean((format && allowedImageExtensions.has(format)) || (fm && allowedImageExtensions.has(fm)));
}

async function validateImageUrl(urlText: string): Promise<void> {
  let parsed: URL;
  try {
    parsed = new URL(urlText);
  } catch {
    throw new Error("图片地址格式不正确，请填写完整的 http/https URL");
  }

  if (!["http:", "https:"].includes(parsed.protocol)) {
    throw new Error("图片地址必须使用 http 或 https 协议");
  }

  if (!isLikelyImageUrl(parsed)) {
    throw new Error("图片地址不是常见可访问图片格式，请使用 png/jpg/jpeg/webp/gif/bmp/svg/avif");
  }

  await new Promise<void>((resolve, reject) => {
    const image = new window.Image();
    image.onload = () => resolve();
    image.onerror = () => reject(new Error("该图片无法访问或不是有效图片资源"));
    image.referrerPolicy = "no-referrer";
    image.src = parsed.toString();
  });
}

export default function PublishPage() {
  return (
    <AuthGuard requireAuth>
      <PublishPageContent />
    </AuthGuard>
  );
}

function PublishPageContent() {
  const [step, setStep] = useState(0);
  const [title, setTitle] = useState("");
  const [description, setDescription] = useState("");
  const [price, setPrice] = useState("300");
  const [categoryId, setCategoryId] = useState("1");
  const [tradeMode, setTradeMode] = useState("MEETUP");
  const [location, setLocation] = useState("犀浦校区");
  const [tags, setTags] = useState("99新,可当面验货");
  const [suggestedBidStep, setSuggestedBidStep] = useState("20");
  const [antiSnipingWindow, setAntiSnipingWindow] = useState("300");
  const [extendSeconds, setExtendSeconds] = useState("180");
  const [images, setImages] = useState<string[]>([]);
  const [pendingImageUrl, setPendingImageUrl] = useState("");
  const [validatingImage, setValidatingImage] = useState(false);
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
      const draftImages = dedupeImageUrls(draft.images ?? []);
      const draftAddedImageUrls = dedupeImageUrls(draft.addedImageUrls ?? []);
      setTitle(draft.title ?? "");
      setDescription(draft.description ?? "");
      setPrice(draft.price ?? "300");
      setCategoryId(draft.categoryId ?? "1");
      setTradeMode(draft.tradeMode ?? "MEETUP");
      setLocation(draft.location ?? "犀浦校区");
      setTags(draft.tags ?? "99新,可当面验货");
      setSuggestedBidStep(draft.suggestedBidStep ?? "20");
      setAntiSnipingWindow(draft.antiSnipingWindow ?? "300");
      setExtendSeconds(draft.extendSeconds ?? "180");
      setImages(draftImages);
      setCreatedItemId(draft.createdItemId ?? null);
      setAddedImageUrls(draftAddedImageUrls);
      localStorage.setItem(
        draftStorageKey,
        JSON.stringify({
          ...draft,
          images: draftImages,
          addedImageUrls: draftAddedImageUrls,
        }),
      );
    }
  }, []);

  function saveDraft(next?: Partial<PublishDraft>) {
    const nextImages = dedupeImageUrls(next?.images ?? images);
    const nextAddedImageUrls = dedupeImageUrls(next?.addedImageUrls ?? addedImageUrls);
    const payload = {
      title,
      description,
      price,
      categoryId,
      tradeMode,
      location,
      tags,
      suggestedBidStep,
      antiSnipingWindow,
      extendSeconds,
      images: nextImages,
      createdItemId,
      addedImageUrls: nextAddedImageUrls,
      ...next,
    };
    payload.images = nextImages;
    payload.addedImageUrls = nextAddedImageUrls;
    localStorage.setItem(draftStorageKey, JSON.stringify(payload));
    setToast("草稿已自动保存");
    setToastTone("success");
  }

  function rememberCreatedItem(itemId: string) {
    setCreatedItemId(itemId);
    saveDraft({ createdItemId: itemId });
  }

  async function addImage() {
    const value = pendingImageUrl.trim();
    if (!value) return;
    if (images.includes(value)) {
      setToast("相同图片 URL 已存在，无需重复添加");
      setToastTone("danger");
      return;
    }
    setValidatingImage(true);
    setToast("");
    try {
      await validateImageUrl(value);
    } catch (error) {
      setToast(error instanceof Error ? error.message : "图片校验失败");
      setToastTone("danger");
      setValidatingImage(false);
      return;
    }
    const next = [...images, value];
    setImages(next);
    saveDraft({ images: next });
    setPendingImageUrl("");
    setToast("图片已添加");
    setToastTone("success");
    setValidatingImage(false);
  }

  function removeImage(imageUrl: string) {
    const next = images.filter((image) => image !== imageUrl);
    setImages(next);
    saveDraft({ images: next });
    setToast("图片已移除");
    setToastTone("success");
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
          category_id: Number(categoryId),
          start_price: parseFloat(price) || 0,
          trade_mode: tradeMode,
          location,
          tags_json: tags.split(",").map((tag) => tag.trim()).filter(Boolean),
          suggested_bid_step: parseFloat(suggestedBidStep) || 0,
          suggested_anti_sniping_window_seconds: parseInt(antiSnipingWindow, 10) || 0,
          suggested_extend_seconds: parseInt(extendSeconds, 10) || 0,
          cover_image_url: images[0] ?? "",
          request_id: idempotencyKey,
        } as never);
        itemId = String(created.item_id);
        rememberCreatedItem(itemId);
      }

      let nextAddedImageUrls = [...addedImageUrls];
      for (const [index, image] of images.entries()) {
        if (nextAddedImageUrls.includes(image)) continue;
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
      setToast(err instanceof Error ? err.message : "提交失败");
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
          <p className="mt-2 text-sm leading-6 text-slate-600">本页提交的是拍品与建议竞价配置，审核通过后由管理端创建正式拍卖。</p>
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
              <div className="grid gap-5 sm:grid-cols-2">
                <Field label="拍品标题">
                  <input value={title} onBlur={() => saveDraft()} onChange={(event) => setTitle(event.target.value)} className="h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-500/20" placeholder="例如：iPad Air 5 64G 蓝色" />
                </Field>
                <Field label="分类 ID">
                  <input value={categoryId} onBlur={() => saveDraft()} onChange={(event) => setCategoryId(event.target.value)} className="h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500" />
                </Field>
                <Field label="拍品描述">
                  <textarea value={description} onBlur={() => saveDraft()} onChange={(event) => setDescription(event.target.value)} className="min-h-36 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 py-3 outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-500/20 sm:col-span-2" placeholder="描述成色、配件、交易方式和注意事项" />
                </Field>
              </div>
            ) : null}

            {step === 1 ? (
              <div>
                <div className="rounded-lg border-2 border-dashed border-slate-300 bg-slate-50 p-8 text-center hover:border-indigo-400 hover:bg-indigo-50/40">
                  <UploadCloud className="mx-auto mb-3 h-10 w-10 text-indigo-600" />
                  <p className="font-black text-slate-950">粘贴图片 URL 作为首轮占位上传</p>
                  <div className="mx-auto mt-5 flex w-full max-w-xl gap-3">
                    <input
                      value={pendingImageUrl}
                      onChange={(event: ChangeEvent<HTMLInputElement>) => setPendingImageUrl(event.target.value)}
                      className="h-11 flex-1 rounded-lg border border-slate-200 bg-white px-3 text-sm outline-none focus:border-indigo-500"
                      placeholder="https://images.unsplash.com/..."
                    />
                    <Button onClick={addImage} disabled={validatingImage || !pendingImageUrl.trim()} type="button">
                      {validatingImage ? <Loader2 className="h-4 w-4 animate-spin" /> : <ImagePlus className="h-4 w-4" />}
                      {validatingImage ? "校验中" : "添加图片"}
                    </Button>
                  </div>
                </div>
                <div className="mt-5 grid gap-3 sm:grid-cols-4">
                  {images.map((image, index) => (
                    <div key={`${image}-${index}`} className="relative aspect-square rounded-lg border border-slate-200 bg-cover bg-center" style={{ backgroundImage: `url(${image})` }}>
                      {index === 0 ? <span className="absolute left-2 top-2 rounded-full bg-white/90 px-2 py-1 text-xs font-bold text-indigo-700">封面首图</span> : null}
                      <button
                        type="button"
                        aria-label="移除图片"
                        onClick={() => removeImage(image)}
                        className="absolute right-2 top-2 inline-flex h-7 w-7 items-center justify-center rounded-full bg-slate-950/70 text-white transition hover:bg-rose-600"
                      >
                        <X className="h-4 w-4" />
                      </button>
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
                <Field label="建议最小加价幅度">
                  <input value={suggestedBidStep} onBlur={() => saveDraft()} onChange={(event) => setSuggestedBidStep(event.target.value)} className="tabular h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500" />
                </Field>
                <Field label="建议延时保护窗口（秒）">
                  <input value={antiSnipingWindow} onBlur={() => saveDraft()} onChange={(event) => setAntiSnipingWindow(event.target.value)} className="tabular h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500" />
                </Field>
                <Field label="建议顺延秒数">
                  <input value={extendSeconds} onBlur={() => saveDraft()} onChange={(event) => setExtendSeconds(event.target.value)} className="tabular h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500" />
                </Field>
                <Field label="交易方式">
                  <select value={tradeMode} onChange={(event) => setTradeMode(event.target.value)} onBlur={() => saveDraft()} className="h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500">
                    <option value="MEETUP">校园当面交易</option>
                    <option value="SELF_PICKUP">自提</option>
                    <option value="SHIPPING">可邮寄</option>
                  </select>
                </Field>
                <Field label="交付地点">
                  <input value={location} onBlur={() => saveDraft()} onChange={(event) => setLocation(event.target.value)} className="h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500" />
                </Field>
                <Field label="标签（逗号分隔）">
                  <input value={tags} onBlur={() => saveDraft()} onChange={(event) => setTags(event.target.value)} className="h-11 w-full rounded-lg border border-slate-200 bg-slate-50 px-3 outline-none focus:border-indigo-500 sm:col-span-2" />
                </Field>
              </div>
            ) : null}

            {step === 3 ? (
              <div className="space-y-4">
                {[
                  ["标题", title || "未填写"],
                  ["描述", description || "未填写"],
                  ["起拍价", `¥${price}`],
                  ["交易方式", tradeMode],
                  ["交付地点", location || "未填写"],
                  ["建议加价幅度", `¥${suggestedBidStep}`],
                  ["图片数量", `${images.length} 张`],
                ].map(([label, value]) => (
                  <div key={label} className="flex items-center justify-between rounded-lg border border-slate-200 bg-slate-50 px-4 py-3 text-sm">
                    <span className="font-bold text-slate-600">{label}</span>
                    <span className="font-black text-slate-950">{value}</span>
                  </div>
                ))}
                <div className="flex items-center gap-2 rounded-lg border border-emerald-200 bg-emerald-50 px-4 py-3 text-sm font-bold text-emerald-700">
                  <CheckCircle2 className="h-5 w-5" />
                  {submittedItemId ? `拍品 #${submittedItemId} 已提交审核` : "提交后进入管理员审核队列，由管理端创建正式拍卖"}
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
