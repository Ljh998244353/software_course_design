<template>
  <div class="full-bleed" :style="bgStyle">
    <div v-if="overlay" class="full-bleed-overlay" :style="{ background: overlay }" />
    <div v-if="panel" class="full-bleed-panel" :style="panelStyle">
      <div class="full-bleed-panel-content">
        <slot />
      </div>
    </div>
    <div v-else class="full-bleed-content" :style="{ textAlign: align, padding: contentPadding }">
      <slot />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = defineProps({
  image: { type: String, default: '' },
  backgroundSize: { type: String, default: 'cover' },
  backgroundPosition: { type: String, default: 'center' },
  overlay: { type: String, default: '' },
  panel: { type: Boolean, default: false },
  panelWidth: { type: String, default: '45%' },
  panelSide: { type: String, default: 'left' },
  panelColor: { type: String, default: 'rgba(26, 27, 46, 0.92)' },
  align: { type: String, default: 'left' },
  contentPadding: { type: String, default: '3rem' },
})

const bgStyle = computed(() => {
  if (!props.image) return {}
  return {
    backgroundImage: `url(${props.image})`,
    backgroundSize: props.backgroundSize,
    backgroundPosition: props.backgroundPosition,
    backgroundRepeat: 'no-repeat',
  }
})

const panelStyle = computed(() => ({
  width: props.panelWidth,
  background: props.panelColor,
  [props.panelSide === 'right' ? 'marginLeft' : 'marginRight']: 'auto',
}))
</script>

<style scoped>
.full-bleed {
  width: 100%;
  height: 100%;
  padding: 0 !important;
  margin: 0;
  position: relative;
}

.full-bleed-overlay {
  position: absolute;
  inset: 0;
  z-index: 1;
  pointer-events: none;
}

.full-bleed-content {
  position: relative;
  z-index: 2;
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  justify-content: center;
}

.full-bleed-content :deep(> *) {
  max-width: none;
}

.full-bleed-panel {
  height: 100%;
  display: flex;
  align-items: center;
  position: relative;
  z-index: 2;
  backdrop-filter: blur(2px);
}

.full-bleed-panel-content {
  padding: 3rem;
  width: 100%;
}

.full-bleed-panel-content :deep(h1) {
  position: static;
  height: auto;
  max-height: none;
  font-size: 2rem;
  color: white;
}

.full-bleed-panel-content :deep(p) {
  color: rgba(255, 255, 255, 0.8);
}

.full-bleed-panel-content :deep(li) {
  color: rgba(255, 255, 255, 0.8);
}
</style>
