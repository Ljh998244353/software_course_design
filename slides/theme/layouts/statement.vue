<template>
  <div class="slidev-layout statement">
    <CenteredSlide dark content-max-width="52rem">
      <slot />
    </CenteredSlide>
  </div>
</template>

<script setup lang="ts">
import CenteredSlide from '../components/internal/CenteredSlide.vue'
</script>

<style scoped>
.statement :deep(h1) {
  font-size: 2.5rem;
  line-height: 1.3;
  font-weight: 800;
}

.statement :deep(p) {
  font-size: 1.15rem;
  line-height: 1.6;
  color: var(--text-secondary, #abb2bf);
  margin-top: 1rem;
}
</style>
