<template>
  <kbd class="kbd"><slot /></kbd>
</template>

<style scoped>
.kbd {
  display: inline-flex;
  align-items: center;
  padding: 0.15rem 0.45rem;
  font-family: var(--slidev-theme-font-mono, 'JetBrains Mono', monospace);
  font-size: 0.75em;
  line-height: 1.4;
  color: var(--text-primary, #dce0e8);
  background: var(--bg-tertiary, #2a2e3a);
  border: 1px solid var(--border-default, rgba(255, 255, 255, 0.15));
  border-bottom-width: 2px;
  border-radius: 0.25rem;
  box-shadow: 0 1px 0 rgba(0, 0, 0, 0.2);
  white-space: nowrap;
  vertical-align: middle;
}
</style>
