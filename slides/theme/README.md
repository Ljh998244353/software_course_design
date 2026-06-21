# One Purple Unicorn Pro - Slidev Theme

A professional Slidev theme with purple and indigo gradients, optimized for technical presentations.

## Features

- **Purple & Indigo Color Scheme**: Beautiful gradient effects from purple to blue
- **Light & Dark Modes**: User-configurable theme switching
- **Korean + English Support**: Noto Sans KR font for mixed language presentations
- **Clean Layouts**: Special layouts for cover, intro, section, and end slides
- **Code Highlighting**: Vitesse theme for consistent syntax highlighting

## Installation

```bash
npm install slidev-theme-one-purple-unicorn-pro
```

## Usage

Add the theme to your Slidev presentation:

```yaml
---
theme: slidev-theme-one-purple-unicorn-pro
colorSchema: light  # or 'dark', 'auto'
---

# Your Presentation Title
```

## Theme Modes

### Light Mode (Default)
```yaml
---
colorSchema: light
---
```

### Dark Mode
```yaml
---
colorSchema: dark
---
```

### Auto Mode (System Preference)
```yaml
---
colorSchema: auto
---
```

You can also switch themes per slide:
```yaml
---
colorSchema: dark
---

# This slide uses dark mode
```

## Available Layouts

### Cover Layout
```yaml
---
layout: cover
---

# Your Title
Subtitle or description
```

### Section Layout
```yaml
---
layout: section
---

# Section Title
```

### Intro Layout
```yaml
---
layout: intro
---

# Introduction
Your intro content here
```

### End Layout
```yaml
---
layout: end
---

# Thank You
Contact information
```

### Two Columns Layout
```yaml
---
layout: two-cols
---

# Two Column Layout

::left::

Left column content

::right::

Right column content
```

## Color Palette

- **Primary**: `#8b5cf6` (Purple)
- **Secondary**: `#6366f1` (Indigo)
- **Accent**: `#ec4899` (Pink)
- **Gradient**: Purple → Blue

## Typography

- **Sans**: Noto Sans KR, Inter, system-ui
- **Mono**: JetBrains Mono, Fira Code
- **Serif**: Noto Serif KR

## CSS Variables

You can override theme variables in your slides:

```css
:root {
  --primary: #8b5cf6;
  --primary-light: #a78bfa;
  --primary-dark: #7c3aed;
  --secondary: #6366f1;
  --accent: #ec4899;
  --gradient: linear-gradient(135deg, var(--primary), var(--secondary));
}
```

## License

MIT