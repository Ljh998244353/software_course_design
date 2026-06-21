<template>
  <div class="media-panel" :class="`media-${side}`">
    <div v-if="side === 'left'" class="media-container" :style="mediaStyle">
      <iframe v-if="type === 'iframe'" :src="src" :style="iframeScale" frameborder="0" allowfullscreen />
    </div>
    <div class="media-content">
      <slot />
    </div>
    <div v-if="side === 'right'" class="media-container" :style="mediaStyle">
      <iframe v-if="type === 'iframe'" :src="src" :style="iframeScale" frameborder="0" allowfullscreen />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = defineProps({
  src: { type: String, required: true },
  type: { type: String, default: 'image' },
  side: { type: String, default: 'left' },
  backgroundSize: { type: String, default: 'cover' },
  scale: { type: [Number, String], default: 1 },
})

const mediaStyle = computed(() => {
  if (props.type === 'image') {
    return {
      backgroundImage: `url(${props.src})`,
      backgroundSize: props.backgroundSize,
      backgroundPosition: 'center',
      backgroundRepeat: 'no-repeat',
    }
  }
  return {}
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
.media-panel {
  display: grid;
  grid-template-columns: 1fr 1fr;
  width: 100%;
  height: 100%;
}

.media-container {
  width: 100%;
  height: 100%;
  overflow: hidden;
}

.media-container iframe {
  border: none;
}

.media-content {
  padding: 2rem;
  overflow: auto;
}
</style>
