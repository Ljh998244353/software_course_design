<template>
  <div class="qrcode" :class="`qrcode-${size}`">
    <img
      :src="`https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=${encodeURIComponent(url)}`"
      :alt="`QR: ${url}`"
      class="qrcode-img"
    />
    <div v-if="caption" class="qrcode-caption">{{ caption }}</div>
  </div>
</template>

<script setup lang="ts">
defineProps({
  url: { type: String, required: true },
  caption: { type: String, default: '' },
  size: { type: String, default: 'default' },
})
</script>

<style scoped>
.qrcode {
  display: inline-flex;
  flex-direction: column;
  align-items: center;
  gap: 0.5rem;
}

.qrcode-img {
  border-radius: 0.5rem;
  background: white;
  padding: 0.5rem;
  border: 1px solid var(--rule, #d9d7cf);
  box-shadow: var(--shadow-md, 0 2px 6px rgba(26, 31, 43, 0.08));
}

.qrcode-caption {
  font-size: 0.7rem;
  color: var(--ink-muted, #6b7280);
  text-align: center;
}

/* Sizes */
.qrcode-sm .qrcode-img { width: 80px; height: 80px; }
.qrcode-default .qrcode-img { width: 120px; height: 120px; }
.qrcode-lg .qrcode-img { width: 180px; height: 180px; }
.qrcode-xl .qrcode-img { width: 240px; height: 240px; }
</style>
