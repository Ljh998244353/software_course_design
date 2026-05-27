import { WifiOff } from "lucide-react";

export function OfflineBanner({ text = "实时通道已降级，当前价格以刷新后的权威数据为准。" }: { text?: string }) {
  return (
    <div className="border-b border-amber-200 bg-amber-50 px-4 py-2 text-sm font-medium text-amber-800">
      <div className="mx-auto flex max-w-7xl items-center gap-2">
        <WifiOff className="h-4 w-4" />
        {text}
      </div>
    </div>
  );
}
