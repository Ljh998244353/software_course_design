<template>
  <div class="slidev-layout full-text">
    <slot />
  </div>
</template>

<style scoped>
.full-text {
  width: 100%;
  height: 100%;
  padding: 2rem 3rem !important;
  margin: 0;
  overflow: auto;
}

.full-text :deep(> *) {
  max-width: none;
}

.full-text :deep(h1) {
  position: static;
  height: auto;
  max-height: none;
  font-size: 1.75rem;
  margin-bottom: 1rem;
}
</style>
