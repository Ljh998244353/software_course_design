<template>
  <div class="slidev-layout full-split">
    <FullBleed
      :image="image"
      :background-size="backgroundSize"
      :background-position="backgroundPosition"
      panel
      :panel-width="panelWidth"
      :panel-side="panelSide"
      :panel-color="panelColor"
    >
      <slot />
    </FullBleed>
  </div>
</template>

<script setup lang="ts">
import FullBleed from '../components/internal/FullBleed.vue'

defineProps({
  image: { type: String, default: '' },
  backgroundSize: { type: String, default: 'cover' },
  backgroundPosition: { type: String, default: 'center' },
  panelWidth: { type: String, default: '45%' },
  panelSide: { type: String, default: 'left' },
  panelColor: { type: String, default: 'rgba(26, 27, 46, 0.92)' },
})
</script>
