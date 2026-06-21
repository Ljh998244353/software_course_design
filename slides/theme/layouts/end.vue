<template>
  <div class="slidev-layout end">
    <CenteredSlide dark content-max-width="48rem">
      <slot />
    </CenteredSlide>
  </div>
</template>

<script setup lang="ts">
import CenteredSlide from '../components/internal/CenteredSlide.vue'
</script>
