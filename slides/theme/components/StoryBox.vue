<template>
  <div class="story-box" :class="`story-${variant}`">
    <div class="story-header">
      <span class="story-kicker">{{ labelMap[variant] || labelMap.history }}</span>
      <span class="story-title">{{ title }}</span>
      <span v-if="year" class="story-year">{{ year }}</span>
    </div>
    <div class="story-body">
      <slot />
    </div>
    <div v-if="source" class="story-source">
      &mdash; {{ source }}
    </div>
  </div>
</template>

<script setup lang="ts">
defineProps({
  title: { type: String, default: 'History' },
  variant: { type: String, default: 'history' },
  year: { type: String, default: '' },
  source: { type: String, default: '' },
})

const labelMap: Record<string, string> = {
  history: 'History',
  insight: 'Insight',
  warning: 'Caution',
  tip: 'Note',
  person: 'Profile',
}
</script>

<style scoped>
.story-box {
  border-radius: 0 0.4rem 0.4rem 0;
  padding: 0.75rem 1rem;
  margin: 0.75rem 0;
  border-left: 3px solid;
  background: var(--paper-2);
  font-size: 0.8rem;
}

.story-history {
  border-color: var(--accent);
}
.story-history .story-kicker { color: var(--accent); }

.story-insight {
  border-color: var(--accent-2);
}
.story-insight .story-kicker { color: var(--accent-2); }

.story-warning {
  border-color: var(--warning, #b4791f);
}
.story-warning .story-kicker { color: var(--warning, #b4791f); }

.story-tip {
  border-color: var(--success, #3f7d4e);
}
.story-tip .story-kicker { color: var(--success, #3f7d4e); }

.story-person {
  border-color: var(--accent);
}
.story-person .story-kicker { color: var(--accent); }

.story-header {
  display: flex;
  align-items: baseline;
  gap: 0.5rem;
  margin-bottom: 0.4rem;
}

.story-kicker {
  font-family: 'Inter', 'Noto Sans KR', sans-serif;
  text-transform: uppercase;
  letter-spacing: 0.12em;
  font-size: 0.6rem;
  font-weight: 700;
}

.story-title {
  font-family: var(--font-serif);
  font-weight: 700;
  font-size: 0.85rem;
  color: var(--ink);
}

.story-year {
  font-family: var(--slidev-theme-font-mono, 'JetBrains Mono', monospace);
  font-size: 0.65rem;
  color: var(--ink-muted);
  background: var(--paper-3);
  padding: 0.1rem 0.4rem;
  border-radius: 0.25rem;
  margin-left: auto;
}

.story-body {
  color: var(--ink-2);
  line-height: 1.5;
}

.story-body :deep(p) {
  margin: 0.2rem 0;
  font-size: 0.8rem;
}

.story-body :deep(strong) {
  color: var(--text-primary, #dce0e8);
}

.story-source {
  font-size: 0.7rem;
  color: var(--text-muted, #5c6370);
  font-style: italic;
  margin-top: 0.4rem;
}
</style>
