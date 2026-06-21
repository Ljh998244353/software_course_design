<template>
  <div class="complexity-table">
    <div v-if="title" class="ct-title">{{ title }}</div>
    <table>
      <thead>
        <tr>
          <th>N</th>
          <th>Target</th>
          <th>Algorithm</th>
          <th v-if="showComplexity">Complexity</th>
        </tr>
      </thead>
      <tbody>
        <tr
          v-for="(row, i) in rows"
          :key="`row-${row.n}-${i}`"
          :class="{ 'ct-highlight': row.highlight }"
        >
          <td class="ct-n">{{ row.n }}</td>
          <td class="ct-target">{{ row.target }}</td>
          <td class="ct-algo">{{ row.algo }}</td>
          <td v-if="showComplexity" class="ct-complexity">{{ row.complexity }}</td>
        </tr>
      </tbody>
    </table>
  </div>
</template>

<script lang="ts">
interface Row {
  n: string
  target: string
  algo: string
  complexity?: string
  highlight?: boolean
}

const DEFAULT_ROWS: Row[] = [
  { n: 'N ≤ 10', target: 'O(N!)', algo: 'Brute Force / Permutation', complexity: 'O(N!)' },
  { n: 'N ≤ 20', target: 'O(2^N)', algo: 'Bitmask DP / Backtracking', complexity: 'O(2^N · N)' },
  { n: 'N ≤ 500', target: 'O(N³)', algo: 'Floyd-Warshall / DP', complexity: 'O(N³)' },
  { n: 'N ≤ 5,000', target: 'O(N²)', algo: 'DP / Brute Force', complexity: 'O(N²)' },
  { n: 'N ≤ 100,000', target: 'O(N log N)', algo: 'Sort / Segment Tree', complexity: 'O(N log N)' },
  { n: 'N ≤ 1,000,000', target: 'O(N)', algo: 'Greedy / Linear Scan', complexity: 'O(N)' },
  { n: 'N ≤ 10^18', target: 'O(log N)', algo: 'Binary Search / Math', complexity: 'O(log N)' },
]
</script>

<script setup lang="ts">
defineProps({
  title: { type: String, default: '' },
  rows: {
    type: Array as () => Row[],
    default: () => DEFAULT_ROWS,
  },
  showComplexity: { type: Boolean, default: true },
})
</script>

<style scoped>
.complexity-table {
  margin: 0.75rem 0;
  border-radius: 0.75rem;
  overflow: hidden;
  border: 1px solid var(--border-default, rgba(255, 255, 255, 0.1));
  background: var(--bg-secondary, #1e2030);
}

.ct-title {
  font-weight: 700;
  font-size: 0.8rem;
  padding: 0.5rem 1rem;
  background: rgba(198, 120, 221, 0.1);
  color: var(--primary-400, #c678dd);
  border-bottom: 1px solid var(--border-default, rgba(255, 255, 255, 0.06));
  letter-spacing: 0.02em;
}

table {
  width: 100%;
  border-collapse: collapse;
  font-size: 0.75rem;
  margin: 0;
  box-shadow: none;
  background: transparent;
}

thead {
  background: var(--bg-tertiary, rgba(255, 255, 255, 0.03));
}

th {
  padding: 0.45rem 0.75rem;
  text-align: left;
  font-weight: 600;
  font-size: 0.65rem;
  text-transform: uppercase;
  letter-spacing: 0.06em;
  color: var(--text-muted, #5c6370);
  border-bottom: 1px solid var(--border-default, rgba(255, 255, 255, 0.06));
}

td {
  padding: 0.4rem 0.75rem;
  border-top: 1px solid var(--border-default, rgba(255, 255, 255, 0.03));
  color: var(--text-secondary, #abb2bf);
}

tbody tr:hover {
  background: var(--bg-tertiary, rgba(255, 255, 255, 0.02));
}

.ct-highlight {
  background: rgba(198, 120, 221, 0.08) !important;
}

.ct-highlight td {
  color: var(--text-primary, #dce0e8);
}

.ct-n {
  font-family: var(--slidev-theme-font-mono, 'JetBrains Mono', monospace);
  font-weight: 600;
  color: var(--one-dark-cyan, #56b6c2);
  white-space: nowrap;
}

.ct-target {
  font-family: var(--slidev-theme-font-mono, 'JetBrains Mono', monospace);
  color: var(--one-dark-yellow, #e5c07b);
  white-space: nowrap;
}

.ct-algo {
  color: var(--text-primary, #dce0e8);
}

.ct-complexity {
  font-family: var(--slidev-theme-font-mono, 'JetBrains Mono', monospace);
  color: var(--one-dark-green, #98c379);
  white-space: nowrap;
}
</style>
