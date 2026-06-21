<template>
  <div class="slidev-layout intro">
    <CenteredSlide dark content-max-width="52rem" align="left">
      <slot />
    </CenteredSlide>
  </div>
</template>

<script setup lang="ts">
import CenteredSlide from '../components/internal/CenteredSlide.vue'
</script>
