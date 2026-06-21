<template>
  <div class="slidev-layout image-layout">
    <MediaPanel :src="image" type="image" :side="side" :background-size="backgroundSize">
      <slot />
    </MediaPanel>
  </div>
</template>

<script setup lang="ts">
import MediaPanel from '../components/internal/MediaPanel.vue'

defineProps({
  image: { type: String, required: true },
  side: { type: String, default: 'left' },
  backgroundSize: { type: String, default: 'cover' },
})
</script>
