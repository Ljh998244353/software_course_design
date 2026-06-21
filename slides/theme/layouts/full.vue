<template>
  <div class="slidev-layout full">
    <slot />
  </div>
</template>

<style scoped>
.full {
  width: 100%;
  height: 100%;
  padding: 0 !important;
  margin: 0;
  overflow: hidden;
}

.full :deep(> *) {
  max-width: none;
}
</style>
