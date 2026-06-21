<template>
  <div class="pattern-card">
    <div v-if="signal" class="pattern-section pattern-signal">
      <div class="pattern-icon">&#x1F6A8;</div>
      <div class="pattern-label">Signal</div>
      <div class="pattern-body">{{ signal }}</div>
    </div>
    <div v-if="template" class="pattern-section pattern-template">
      <div class="pattern-icon">&#x1F4DD;</div>
      <div class="pattern-label">Template</div>
      <div class="pattern-body">
        <slot name="template">{{ template }}</slot>
      </div>
    </div>
    <div v-if="alternatives" class="pattern-section pattern-alt">
      <div class="pattern-icon">&#x1F500;</div>
      <div class="pattern-label">Alternatives</div>
      <div class="pattern-body">
        <slot name="alternatives">{{ alternatives }}</slot>
      </div>
    </div>
    <slot />
  </div>
</template>

<script setup lang="ts">
defineProps({
  signal: { type: String, default: '' },
  template: { type: String, default: '' },
  alternatives: { type: String, default: '' },
})
</script>

<style scoped>
.pattern-card {
  border: 1px solid var(--border-default, rgba(255, 255, 255, 0.1));
  border-radius: 0.75rem;
  overflow: hidden;
  margin: 0.75rem 0;
  background: var(--bg-secondary, #1e2030);
}

.pattern-section {
  padding: 0.6rem 1rem;
  display: flex;
  align-items: flex-start;
  gap: 0.5rem;
  border-bottom: 1px solid var(--border-default, rgba(255, 255, 255, 0.06));
}

.pattern-section:last-of-type {
  border-bottom: none;
}

.pattern-icon {
  font-size: 0.85rem;
  flex-shrink: 0;
  margin-top: 0.1rem;
}

.pattern-label {
  font-family: var(--slidev-theme-font-mono, 'JetBrains Mono', monospace);
  font-size: 0.65rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.08em;
  min-width: 5.5rem;
  flex-shrink: 0;
  margin-top: 0.15rem;
}

.pattern-signal .pattern-label {
  color: var(--one-dark-red, #e06c75);
}

.pattern-template .pattern-label {
  color: var(--one-dark-blue, #61afef);
}

.pattern-alt .pattern-label {
  color: var(--one-dark-yellow, #e5c07b);
}

.pattern-body {
  font-size: 0.8rem;
  line-height: 1.5;
  color: var(--text-secondary, #abb2bf);
}

.pattern-body :deep(code) {
  font-size: 0.75rem;
}

.pattern-signal {
  background: rgba(224, 108, 117, 0.06);
}

.pattern-template {
  background: rgba(97, 175, 239, 0.06);
}

.pattern-alt {
  background: rgba(229, 192, 123, 0.06);
}
</style>
