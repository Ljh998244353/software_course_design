<template>
  <div
    v-if="!isHidden && stages.length > 0"
    class="stage-progress"
    :class="`stage-pos-${position}`"
  >
    <div
      v-for="(stage, i) in stages"
      :key="`stage-${stage}-${i}`"
      class="stage-item stage-clickable"
      :class="{
        'stage-done': i < currentIndex,
        'stage-active': i === currentIndex,
        'stage-upcoming': i > currentIndex,
      }"
      @click="goToStage(i)"
    >
      <div class="stage-dot" />
      <span class="stage-label">{{ stage }}</span>
      <div v-if="i < stages.length - 1" class="stage-connector" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useNav } from '@slidev/client'

const nav = useNav()
const { currentLayout, currentSlideRoute, go } = nav

const hiddenLayouts = ['cover', 'intro', 'end']

const isHidden = computed(() => hiddenLayouts.includes(currentLayout.value))

const frontmatter = computed(() =>
  currentSlideRoute.value?.meta?.slide?.frontmatter || {}
)

const stages = computed(() => (frontmatter.value.stages as string[]) || [])

const currentIndex = computed(() => {
  const val = frontmatter.value.currentStage
  return val !== undefined ? (val as number) : -1
})

const position = computed(() =>
  (frontmatter.value.stagePosition as string) || 'top'
)

// Pre-compute stages key for comparison (avoids JSON.stringify per click)
const stagesKey = computed(() => stages.value.join('|||'))

function goToStage(index: number) {
  // Explicit stageMap takes priority
  const map = frontmatter.value.stageMap as number[] | undefined
  if (map && map[index] !== undefined) {
    go(map[index])
    return
  }

  // Auto-detect: scan slide routes for matching stages + currentStage
  const slides = (nav as any).slides?.value || (nav as any).navState?.slides
  if (!slides) return

  const key = stagesKey.value
  for (const slide of slides) {
    const fm = slide?.meta?.slide?.frontmatter
    if (!fm?.stages) continue
    if (fm.stages.join('|||') === key && fm.currentStage === index) {
      go(slide.no ?? slide.idx + 1)
      return
    }
  }
}
</script>

<style scoped>
.stage-progress {
  position: fixed;
  display: flex;
  align-items: center;
  gap: 0;
  z-index: 10;
  font-family: var(--slidev-theme-font-mono, 'JetBrains Mono', monospace);
}

/* Position variants */
.stage-pos-top {
  top: 1rem;
  left: 50%;
  transform: translateX(-50%);
  flex-direction: row;
}

.stage-pos-top-left {
  top: 1rem;
  left: 2rem;
  flex-direction: row;
}

.stage-pos-top-right {
  top: 1rem;
  right: 2rem;
  flex-direction: row;
}

.stage-pos-bottom {
  bottom: 2.5rem;
  left: 50%;
  transform: translateX(-50%);
  flex-direction: row;
}

.stage-pos-left {
  top: 50%;
  left: 1.5rem;
  transform: translateY(-50%);
  flex-direction: column;
}

.stage-pos-left .stage-connector {
  width: 1px;
  height: 1.5rem;
  margin: 0.25rem 0;
}

.stage-pos-left .stage-item {
  flex-direction: column;
  gap: 0.15rem;
}

.stage-pos-right {
  top: 50%;
  right: 1.5rem;
  transform: translateY(-50%);
  flex-direction: column;
}

.stage-pos-right .stage-connector {
  width: 1px;
  height: 1.5rem;
  margin: 0.25rem 0;
}

.stage-pos-right .stage-item {
  flex-direction: column;
  gap: 0.15rem;
}

/* Base item */
.stage-item {
  display: flex;
  align-items: center;
  gap: 0.3rem;
  position: relative;
}

.stage-clickable {
  cursor: pointer;
  pointer-events: auto;
}

.stage-clickable:hover .stage-dot {
  transform: scale(1.4);
}

.stage-clickable:hover .stage-label {
  opacity: 1 !important;
}

/* Dot */
.stage-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  flex-shrink: 0;
  transition: all var(--transition-base, 200ms ease);
}

.stage-label {
  font-size: 0.6rem;
  letter-spacing: 0.03em;
  white-space: nowrap;
  transition: all var(--transition-base, 200ms ease);
}

.stage-connector {
  width: 1.5rem;
  height: 1px;
  margin: 0 0.25rem;
  transition: background var(--transition-base, 200ms ease);
}

/* States */
.stage-done .stage-dot {
  background: var(--one-dark-green, #98c379);
}

.stage-done .stage-label {
  color: var(--one-dark-green, #98c379);
  opacity: 0.7;
}

.stage-done .stage-connector {
  background: var(--one-dark-green, #98c379);
  opacity: 0.4;
}

.stage-active .stage-dot {
  background: var(--accent, #1e3a5f);
  width: 8px;
  height: 8px;
}

.stage-active .stage-label {
  color: var(--accent, #1e3a5f);
  font-weight: 600;
  font-size: 0.65rem;
}

.stage-active .stage-connector {
  background: var(--rule, rgba(0, 0, 0, 0.12));
}

.stage-upcoming .stage-dot {
  background: var(--text-muted, #5c6370);
  opacity: 0.4;
}

.stage-upcoming .stage-label {
  color: var(--text-muted, #5c6370);
  opacity: 0.4;
}

.stage-upcoming .stage-connector {
  background: var(--text-muted, #5c6370);
  opacity: 0.2;
}
</style>
