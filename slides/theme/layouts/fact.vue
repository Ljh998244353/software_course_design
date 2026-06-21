<template>
  <div class="slidev-layout fact">
    <CenteredSlide dark content-max-width="48rem">
      <slot />
    </CenteredSlide>
  </div>
</template>

<script setup lang="ts">
import CenteredSlide from '../components/internal/CenteredSlide.vue'
</script>

<style scoped>
.fact :deep(h1) {
  font-size: 4rem;
  font-weight: 900;
  line-height: 1.1;
  margin-bottom: 1rem;
}

.fact :deep(p) {
  font-size: 1.1rem;
  color: var(--text-secondary, #abb2bf);
  line-height: 1.5;
}

.fact :deep(h2) {
  font-size: 1.2rem;
  font-weight: 600;
  color: var(--text-muted, #5c6370);
  text-transform: uppercase;
  letter-spacing: 0.1em;
  margin-bottom: 0.5rem;
}
</style>
