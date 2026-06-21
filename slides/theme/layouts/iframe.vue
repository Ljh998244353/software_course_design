<template>
  <div class="slidev-layout iframe-layout">
    <div v-if="side === 'full'" class="iframe-full">
      <iframe :src="url" :style="iframeScale" frameborder="0" allowfullscreen />
    </div>
    <MediaPanel v-else :src="url" type="iframe" :side="side" :scale="scale">
      <slot />
    </MediaPanel>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import MediaPanel from '../components/internal/MediaPanel.vue'

const props = defineProps({
  url: { type: String, required: true },
  side: { type: String, default: 'full' },
  scale: { type: [Number, String], default: 1 },
})

const iframeScale = computed(() => {
  const s = Number(props.scale)
  return {
    width: `${100 / s}%`,
    height: `${100 / s}%`,
    transform: `scale(${s})`,
    transformOrigin: '0 0',
  }
})
</script>

<style scoped>
.iframe-full {
  width: 100%;
  height: 100%;
  overflow: hidden;
}

.iframe-full iframe {
  border: none;
}
</style>
