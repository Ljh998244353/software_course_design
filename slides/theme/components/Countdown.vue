<template>
  <div v-if="remaining > 0 || showZero" class="countdown" :class="`countdown-${size}`">
    <span class="countdown-time">{{ formatted }}</span>
    <span v-if="label" class="countdown-label">{{ label }}</span>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'

const props = defineProps({
  minutes: { type: Number, default: 5 },
  autoStart: { type: Boolean, default: true },
  label: { type: String, default: '' },
  size: { type: String, default: 'default' },
  showZero: { type: Boolean, default: true },
})

const remaining = ref(props.minutes * 60)
let timer: ReturnType<typeof setInterval> | null = null

const formatted = computed(() => {
  const m = Math.floor(remaining.value / 60)
  const s = remaining.value % 60
  return `${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}`
})

function start() {
  if (timer) return
  timer = setInterval(() => {
    if (remaining.value > 0) remaining.value--
    else if (timer) { clearInterval(timer); timer = null }
  }, 1000)
}

function stop() {
  if (timer) { clearInterval(timer); timer = null }
}

onMounted(() => {
  if (props.autoStart) start()
})

onUnmounted(() => stop())

defineExpose({ start, stop, remaining })
</script>

<style scoped>
.countdown {
  display: inline-flex;
  align-items: baseline;
  gap: 0.5rem;
  font-family: var(--slidev-theme-font-mono, 'JetBrains Mono', monospace);
}

.countdown-time {
  font-weight: 700;
  font-variant-numeric: tabular-nums;
  color: var(--primary-400, #c678dd);
}

.countdown-label {
  font-size: 0.75em;
  color: var(--text-muted, #5c6370);
}

/* Sizes */
.countdown-default .countdown-time { font-size: 1.5rem; }
.countdown-sm .countdown-time { font-size: 1rem; }
.countdown-lg .countdown-time { font-size: 3rem; }
.countdown-xl .countdown-time { font-size: 5rem; }

@media print {
  .countdown { display: none !important; }
}
</style>
