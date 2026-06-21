<template>
  <a :href="href" target="_blank" rel="noopener noreferrer" class="link-card" :class="{ 'link-card-compact': compact }">
    <div v-if="icon" class="link-icon">{{ icon }}</div>
    <div class="link-body">
      <div class="link-title">{{ title }}</div>
      <div v-if="description" class="link-desc">{{ description }}</div>
    </div>
    <div class="link-arrow">→</div>
  </a>
</template>

<script setup lang="ts">
defineProps({
  href: { type: String, required: true },
  title: { type: String, required: true },
  description: { type: String, default: '' },
  icon: { type: String, default: '' },
  compact: { type: Boolean, default: false },
})
</script>

<style scoped>
.link-card {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  padding: 0.75rem 1rem;
  background: var(--bg-secondary, #1e2030);
  border: 1px solid var(--border-default, rgba(255, 255, 255, 0.1));
  border-radius: 0.75rem;
  text-decoration: none;
  color: inherit;
  transition: all 0.2s ease;
  margin: 0.5rem 0;
}

.link-card::after {
  display: none;
}

.link-card:hover {
  border-color: var(--primary-600, #7c4dbc);
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
}

.link-card-compact {
  padding: 0.5rem 0.75rem;
}

.link-icon {
  font-size: 1.25rem;
  flex-shrink: 0;
}

.link-card-compact .link-icon {
  font-size: 1rem;
}

.link-body {
  flex: 1;
  min-width: 0;
}

.link-title {
  font-size: 0.85rem;
  font-weight: 600;
  color: var(--text-primary, #dce0e8);
}

.link-card-compact .link-title {
  font-size: 0.75rem;
}

.link-desc {
  font-size: 0.7rem;
  color: var(--text-muted, #5c6370);
  margin-top: 0.15rem;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.link-arrow {
  font-size: 0.85rem;
  color: var(--text-muted, #5c6370);
  flex-shrink: 0;
  transition: transform 0.2s ease, color 0.2s ease;
}

.link-card:hover .link-arrow {
  transform: translateX(3px);
  color: var(--primary-400, #c678dd);
}
</style>
