<template>
  <div class="centered-slide" :class="{ 'centered-dark': dark }" :style="{ padding }">
    <div class="centered-content" :style="{ maxWidth: contentMaxWidth, textAlign: align }">
      <slot />
    </div>
  </div>
</template>

<script setup lang="ts">
defineProps({
  dark: { type: Boolean, default: false },
  padding: { type: String, default: '4rem 6rem' },
  contentMaxWidth: { type: String, default: '52rem' },
  align: { type: String, default: 'center' },
})
</script>

<style scoped>
.centered-slide {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}

/* Light-first: special layouts use paper background */
.centered-dark {
  background: var(--paper, #fcfcfa);
  color: var(--ink, #1a1f2b);
}

/* Restore dark background only in real dark mode */
:global(html.dark) .centered-dark {
  background: #1a1b26;
  color: var(--dark-fg-bright, #c0caf5);
}

.centered-content {
  width: 100%;
}
</style>
