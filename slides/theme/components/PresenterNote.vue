<template>
  <div class="presenter-note">
    <div class="pn-label">Note</div>
    <div class="pn-body">
      <slot />
    </div>
  </div>
</template>

<style scoped>
.presenter-note {
  display: none;
}

/* Only visible in presenter mode or when explicitly shown */
.slidev-page.presenter .presenter-note,
.presenter-note.show {
  display: flex;
  gap: 0.5rem;
  padding: 0.5rem 0.75rem;
  background: var(--accent-soft);
  border: 1px dashed var(--accent, #1e3a5f);
  border-radius: 0.5rem;
  margin: 0.5rem 0;
  font-size: 0.75rem;
  color: var(--ink-2, #3f4654);
}

.pn-label {
  flex-shrink: 0;
  font-family: 'Inter', 'Noto Sans KR', sans-serif;
  text-transform: uppercase;
  letter-spacing: 0.12em;
  font-size: 0.6rem;
  font-weight: 700;
  color: var(--accent, #1e3a5f);
}

.pn-body :deep(p) {
  font-size: 0.75rem;
  margin: 0;
}

@media print {
  .presenter-note {
    display: none !important;
  }
}
</style>
