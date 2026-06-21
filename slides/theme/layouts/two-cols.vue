<template>
  <div class="slidev-layout two-cols-layout">
    <div class="col-header">
      <slot />
    </div>
    <GridColumns
      :count="2"
      :widths="[leftWidth, rightWidth]"
      :divider="divider"
      :labels="[leftLabel, rightLabel]"
    >
      <template #left><slot name="left" /></template>
      <template #right><slot name="right" /></template>
    </GridColumns>
    <div class="col-bottom">
      <slot name="bottom" />
    </div>
  </div>
</template>

<script setup lang="ts">
import GridColumns from '../components/internal/GridColumns.vue'

defineProps({
  leftWidth: { type: String, default: '1fr' },
  rightWidth: { type: String, default: '1fr' },
  divider: { type: Boolean, default: false },
  leftLabel: { type: String, default: '' },
  rightLabel: { type: String, default: '' },
})
</script>

<style scoped>
.two-cols-layout {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.col-header { flex-shrink: 0; }
.col-bottom { flex-shrink: 0; }
</style>
