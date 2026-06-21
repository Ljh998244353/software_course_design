<template>
  <div class="slidev-layout three-cols-layout">
    <div class="col-header">
      <slot />
    </div>
    <GridColumns
      :count="3"
      :widths="[leftWidth, centerWidth, rightWidth]"
      :divider="divider"
    >
      <template #left><slot name="left" /></template>
      <template #center><slot name="center" /></template>
      <template #right><slot name="right" /></template>
    </GridColumns>
  </div>
</template>

<script setup lang="ts">
import GridColumns from '../components/internal/GridColumns.vue'

defineProps({
  leftWidth: { type: String, default: '1fr' },
  centerWidth: { type: String, default: '1fr' },
  rightWidth: { type: String, default: '1fr' },
  divider: { type: Boolean, default: false },
})
</script>

<style scoped>
.three-cols-layout {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.col-header { flex-shrink: 0; }
</style>
