<template>
  <div class="slidev-layout center">
    <CenteredSlide padding="4rem" content-max-width="64rem">
      <slot />
    </CenteredSlide>
  </div>
</template>

<script setup lang="ts">
import CenteredSlide from '../components/internal/CenteredSlide.vue'
</script>
