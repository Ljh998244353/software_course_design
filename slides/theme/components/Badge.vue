<template>
  <span class="badge" :class="`badge-${variant}`">
    <slot />
  </span>
</template>

<script setup lang="ts">
defineProps({
  variant: { type: String, default: 'default' },
})
</script>

<style scoped>
.badge {
  display: inline-flex;
  align-items: center;
  padding: 0.15rem 0.5rem;
  font-size: 0.7rem;
  font-weight: 600;
  border-radius: 1rem;
  letter-spacing: 0.02em;
  white-space: nowrap;
  vertical-align: middle;
}

.badge-default {
  background: rgba(198, 120, 221, 0.15);
  color: var(--primary-400, #c678dd);
}

.badge-blue {
  background: rgba(97, 175, 239, 0.15);
  color: var(--one-dark-blue, #61afef);
}

.badge-green {
  background: rgba(152, 195, 121, 0.15);
  color: var(--one-dark-green, #98c379);
}

.badge-red {
  background: rgba(224, 108, 117, 0.15);
  color: var(--one-dark-red, #e06c75);
}

.badge-yellow {
  background: rgba(229, 192, 123, 0.15);
  color: var(--one-dark-yellow, #e5c07b);
}

.badge-cyan {
  background: rgba(86, 182, 194, 0.15);
  color: var(--one-dark-cyan, #56b6c2);
}

.badge-gray {
  background: var(--bg-tertiary, rgba(255, 255, 255, 0.08));
  color: var(--text-muted, #5c6370);
}
</style>
