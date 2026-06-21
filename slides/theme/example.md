---
theme: ./
title: 'One Purple Unicorn Pro Theme Example'
author: Theme Demo
highlighter: shiki
layout: cover
class: text-center
---

# One Purple Unicorn Pro

## A Beautiful Theme for Slidev

Professional presentations with purple/indigo gradients

---
layout: default
---

# Heading Level 1

## Heading Level 2

### Heading Level 3

Regular paragraph text with **bold** and *italic* styling.

- Bullet point 1
- Bullet point 2
  - Nested bullet
  - Another nested bullet
- Bullet point 3

---
layout: two-cols
---

# Two Column Layout

::left::

## Left Column

This is content in the left column.

- Feature 1
- Feature 2
- Feature 3

::right::

## Right Column

This is content in the right column.

```python
def hello_world():
    print("Hello, World!")
    return 42
```

---

# Code Examples

## JavaScript

```javascript
const greet = (name) => {
  console.log(`Hello, ${name}!`);
  return `Welcome to Slidev`;
};
```

## CUDA

```cuda
__global__ void vectorAdd(float *a, float *b, float *c, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        c[idx] = a[idx] + b[idx];
    }
}
```

---

# Tables

| Feature | Description | Status |
|---------|-------------|--------|
| **Dark Mode** | Beautiful dark theme | ✅ |
| **Gradients** | Purple/Indigo colors | ✅ |
| **Code Highlighting** | Syntax highlighting | ✅ |
| **Responsive** | Works on all devices | ✅ |

---

# Math Support

Inline math: $E = mc^2$

Display math:

$$
\frac{1}{N} \sum_{i=1}^{N} x_i = \bar{x}
$$

---
layout: center
class: text-center
---

# Thank You!

### Questions?

[GitHub](https://github.com) · [Twitter](https://twitter.com) · [Email](mailto:example@example.com)