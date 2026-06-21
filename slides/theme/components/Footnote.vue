<template>
  <div class="footnote">
    <slot />
  </div>
</template>

<style scoped>
.footnote {
  position: fixed;
  bottom: 2rem;
  left: 2rem;
  right: 6rem;
  font-size: 0.65rem;
  line-height: 1.4;
  color: var(--text-muted, #5c6370);
  border-top: 1px solid var(--border-default, rgba(255, 255, 255, 0.08));
  padding-top: 0.4rem;
  pointer-events: auto;
  z-index: 5;
}

.footnote :deep(a) {
  color: var(--primary-400, #c678dd);
  font-size: 0.65rem;
  text-decoration: none;
  border-bottom: 1px dashed var(--primary-600, #7c4dbc);
  transition: color 0.2s ease, border-color 0.2s ease;
  pointer-events: auto;
  cursor: pointer;
}

.footnote :deep(a)::after {
  display: none;
}

.footnote :deep(a:hover) {
  color: var(--primary-300, #d8a6e8);
  border-color: var(--primary-400, #c678dd);
}

.footnote :deep(sup) {
  color: var(--primary-400, #c678dd);
  font-weight: 600;
  margin-right: 0.2rem;
}
</style>
