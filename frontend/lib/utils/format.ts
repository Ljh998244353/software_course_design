export function formatPrice(value: number) {
  return new Intl.NumberFormat("zh-CN", {
    style: "currency",
    currency: "CNY",
    maximumFractionDigits: 0
  }).format(value);
}

export function formatTime(value: string) {
  return new Intl.DateTimeFormat("zh-CN", {
    month: "2-digit",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit"
  }).format(new Date(value));
}

export function countdownLabel(value: string) {
  const diff = new Date(value).getTime() - Date.now();
  if (diff <= 0) {
    return "已截标";
  }

  const minutes = Math.floor(diff / 60000);
  const hours = Math.floor(minutes / 60);
  const days = Math.floor(hours / 24);

  if (days > 0) {
    return `${days}天 ${hours % 24}小时`;
  }

  if (hours > 0) {
    return `${hours}小时 ${minutes % 60}分`;
  }

  return `${Math.max(minutes, 1)}分钟`;
}

export function countdownTone(value: string) {
  const diffHours = (new Date(value).getTime() - Date.now()) / 3600000;
  if (diffHours < 0.083) {
    return "text-rose-600 bg-rose-50 border-rose-200";
  }
  if (diffHours <= 12) {
    return "text-amber-700 bg-amber-50 border-amber-200";
  }
  return "text-slate-600 bg-slate-50 border-slate-200";
}
