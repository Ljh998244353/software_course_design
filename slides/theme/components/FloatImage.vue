<template>
  <div
    class="float-image"
    :class="`float-${position}`"
    :style="{ width }"
  >
    <img
      :src="src"
      :alt="alt"
      :style="{ borderRadius: rounded ? '0.5rem' : '0' }"
    />
    <div v-if="caption" class="float-caption">{{ caption }}</div>
  </div>
</template>

<script setup lang="ts">
defineProps({
  src: { type: String, required: true },
  alt: { type: String, default: '' },
  caption: { type: String, default: '' },
  position: { type: String, default: 'right' },
  width: { type: String, default: '40%' },
  rounded: { type: Boolean, default: true },
})
</script>

<style scoped>
.float-image {
  margin-bottom: 0.75rem;
}

.float-right {
  float: right;
  margin-left: 1.25rem;
}

.float-left {
  float: left;
  margin-right: 1.25rem;
}

.float-image img {
  width: 100%;
  height: auto;
  display: block;
  object-fit: cover;
  border: 1px solid var(--border-default, rgba(255, 255, 255, 0.1));
  box-shadow: var(--shadow-md, 0 4px 12px rgba(0, 0, 0, 0.3));
}

.float-caption {
  font-size: 0.65rem;
  color: var(--text-muted, #5c6370);
  text-align: center;
  margin-top: 0.3rem;
  font-style: italic;
}
</style>
