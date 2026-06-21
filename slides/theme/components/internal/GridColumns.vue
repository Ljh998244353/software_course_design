<template>
  <div class="grid-columns" :style="gridStyle">
    <template v-for="(_, i) in count" :key="i">
      <div v-if="divider && i > 0" class="grid-divider" />
      <div class="grid-col">
        <div v-if="labels[i]" class="grid-label" :class="`grid-label-${i}`">{{ labels[i] }}</div>
        <slot :name="slotNames[i]" />
      </div>
    </template>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = defineProps({
  count: { type: Number, default: 2 },
  widths: { type: Array as () => string[], default: () => [] },
  divider: { type: Boolean, default: false },
  labels: { type: Array as () => string[], default: () => [] },
})

const slotNames = computed(() => {
  if (props.count === 2) return ['left', 'right']
  if (props.count === 3) return ['left', 'center', 'right']
  return Array.from({ length: props.count }, (_, i) => `col-${i}`)
})

const gridStyle = computed(() => {
  const w = slotNames.value.map((_, i) => props.widths[i] || '1fr')
  const cols = props.divider
    ? w.join(' 1px ')
    : w.join(' ')
  return { gridTemplateColumns: cols }
})
</script>

<style scoped>
.grid-columns {
  display: grid;
  gap: 1.5rem;
  flex: 1;
  min-height: 0;
}

.grid-divider {
  width: 1px;
  height: 100%;
  background: var(--border-default, rgba(171, 178, 191, 0.15));
}

.grid-col {
  overflow: auto;
}

.grid-label {
  font-family: var(--slidev-theme-font-mono, 'JetBrains Mono', monospace);
  font-size: 0.65rem;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.08em;
  padding: 0.2rem 0.6rem;
  border-radius: 1rem;
  width: fit-content;
  margin-bottom: 0.5rem;
}

.grid-label-0 {
  background: rgba(59, 130, 246, 0.15);
  color: var(--one-dark-blue, #61afef);
}

.grid-label-1 {
  background: rgba(16, 185, 129, 0.15);
  color: var(--one-dark-green, #98c379);
}

.grid-label-2 {
  background: rgba(198, 120, 221, 0.15);
  color: var(--one-dark-magenta, #c678dd);
}
</style>
