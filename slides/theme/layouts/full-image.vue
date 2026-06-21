<template>
  <div class="slidev-layout full-image">
    <FullBleed
      :image="image"
      :background-size="backgroundSize"
      :background-position="position"
      :overlay="overlay"
      align="center"
      content-padding="3rem"
    >
      <slot />
    </FullBleed>
  </div>
</template>

<script setup lang="ts">
import FullBleed from '../components/internal/FullBleed.vue'

defineProps({
  image: { type: String, required: true },
  backgroundSize: { type: String, default: 'cover' },
  overlay: { type: String, default: 'rgba(0, 0, 0, 0.55)' },
  position: { type: String, default: 'center' },
})
</script>

<style scoped>
.full-image :deep(h1) {
  font-size: 3rem;
  font-weight: 900;
  color: white;
  text-shadow: 0 2px 12px rgba(0, 0, 0, 0.5);
}

.full-image :deep(h2) {
  font-size: 1.5rem;
  color: rgba(255, 255, 255, 0.9);
  text-shadow: 0 1px 8px rgba(0, 0, 0, 0.4);
}

.full-image :deep(p) {
  font-size: 1.15rem;
  color: rgba(255, 255, 255, 0.85);
  text-shadow: 0 1px 4px rgba(0, 0, 0, 0.3);
}
</style>
