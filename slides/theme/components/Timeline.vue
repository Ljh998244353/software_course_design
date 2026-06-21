<template>
  <div class="timeline">
    <div
      v-for="(item, i) in items"
      :key="`tl-${i}`"
      class="tl-item"
      :class="{ 'tl-active': i === active }"
    >
      <div class="tl-marker">
        <div class="tl-dot" />
        <div v-if="i < items.length - 1" class="tl-line" />
      </div>
      <div class="tl-content">
        <div v-if="item.year" class="tl-year">{{ item.year }}</div>
        <div class="tl-title">{{ item.title }}</div>
        <div v-if="item.desc" class="tl-desc">{{ item.desc }}</div>
      </div>
    </div>
  </div>
</template>

<script lang="ts">
interface TimelineItem {
  title: string
  year?: string
  desc?: string
}
</script>

<script setup lang="ts">
defineProps({
  items: { type: Array as () => TimelineItem[], required: true },
  active: { type: Number, default: -1 },
})
</script>

<style scoped>
.timeline {
  display: flex;
  flex-direction: column;
  gap: 0;
  margin: 0.75rem 0;
}

.tl-item {
  display: flex;
  gap: 0.75rem;
  min-height: 3rem;
}

.tl-marker {
  display: flex;
  flex-direction: column;
  align-items: center;
  flex-shrink: 0;
  width: 1rem;
}

.tl-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--text-muted, #5c6370);
  flex-shrink: 0;
  margin-top: 0.3rem;
  transition: all 0.2s ease;
}

.tl-active .tl-dot {
  background: var(--accent, #1e3a5f);
  width: 10px;
  height: 10px;
}

.tl-line {
  width: 2px;
  flex: 1;
  background: var(--border-default, rgba(255, 255, 255, 0.1));
  margin: 0.25rem 0;
}

.tl-content {
  padding-bottom: 0.75rem;
}

.tl-year {
  font-family: var(--slidev-theme-font-mono, 'JetBrains Mono', monospace);
  font-size: 0.65rem;
  color: var(--primary-400, #c678dd);
  font-weight: 600;
}

.tl-title {
  font-size: 0.85rem;
  font-weight: 600;
  color: var(--text-primary, #dce0e8);
}

.tl-desc {
  font-size: 0.75rem;
  color: var(--text-muted, #5c6370);
  margin-top: 0.15rem;
}
</style>
