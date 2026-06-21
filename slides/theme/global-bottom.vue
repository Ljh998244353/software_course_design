<template>
  <!-- Progress Bar: always or content-only based on frontmatter -->
  <div v-if="showBar" class="progress-bar">
    <div class="progress-fill" :style="{ width: progress + '%' }" />
  </div>

  <!-- Page Indicator -->
  <div v-if="!isHidden" class="slide-indicator">
    {{ currentPage }} / {{ total }}
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useNav } from '@slidev/client'

const { currentPage, total, currentLayout, currentSlideRoute } = useNav()

const hiddenLayouts = ['cover', 'section', 'intro', 'end']

const isHidden = computed(() => hiddenLayouts.includes(currentLayout.value))

// progressBar frontmatter option:
//   "always" (default) → all slides
//   "content"          → only where page number is shown (hides on cover/section/intro/end)
//   false              → completely off
const progressMode = computed(() => {
  const val = currentSlideRoute.value?.meta?.slide?.frontmatter?.progressBar
  if (val === false || val === 'false') return 'off'
  if (val === 'content') return 'content'
  return 'always'
})

const showBar = computed(() => {
  if (progressMode.value === 'off') return false
  if (progressMode.value === 'content') return !isHidden.value
  return true // always
})

const progress = computed(() =>
  total.value > 1 ? ((currentPage.value - 1) / (total.value - 1)) * 100 : 0
)
</script>

<style scoped>
.progress-bar {
  position: fixed;
  bottom: 0;
  left: 0;
  width: 100%;
  height: 2px;
  background: var(--rule, rgba(0, 0, 0, 0.06));
  z-index: 100;
  pointer-events: none;
}

.progress-fill {
  height: 100%;
  background: var(--accent, #1e3a5f);
  transition: width 300ms ease;
}

.slide-indicator {
  position: fixed;
  right: 2rem;
  bottom: 1.5rem;
  font-family: var(--slidev-theme-font-mono, 'JetBrains Mono', monospace);
  font-size: 0.75rem;
  color: var(--ink-muted, #6b7280);
  letter-spacing: 0.05em;
  opacity: 0.8;
  z-index: 10;
  pointer-events: none;
  transition: opacity var(--transition-fast, 150ms ease);
}
</style>
