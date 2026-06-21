<template>
  <div class="slidev-layout quote">
    <CenteredSlide dark content-max-width="48rem">
      <div class="quote-mark">"</div>
      <div class="quote-content">
        <slot />
      </div>
    </CenteredSlide>
  </div>
</template>

<script setup lang="ts">
import CenteredSlide from '../components/internal/CenteredSlide.vue'
</script>

<style scoped>
.quote-mark {
  font-size: 5rem;
  line-height: 1;
  font-family: var(--font-serif);
  color: var(--accent);
  opacity: 0.35;
  margin-bottom: -1rem;
}

.quote-content :deep(p) {
  font-size: 1.4rem;
  line-height: 1.6;
  font-style: italic;
  font-family: var(--font-serif);
  color: var(--ink);
}

.quote-content :deep(p:last-of-type) {
  font-size: 0.9rem;
  font-style: normal;
  font-family: 'Inter', 'Noto Sans KR', sans-serif;
  color: var(--ink-muted);
  margin-top: 1.5rem;
}

.quote-content :deep(strong) {
  color: var(--accent);
}
</style>
