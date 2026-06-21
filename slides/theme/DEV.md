# One Purple Unicorn Pro Theme Development

## 개발 환경 설정

### 초기 설정 (최초 1회)
```bash
# 테마 디렉토리에서
cd themes/slidev-theme-one-purple-unicorn-pro
npm install
npm run link
```

### 테마 개발 워크플로우

1. **CSS/Vue 파일 수정**
   - `styles/index.css` - 전역 스타일
   - `layouts/*.vue` - 레이아웃 컴포넌트
   - `index.ts` - 테마 설정

2. **변경사항 확인**
   ```bash
   # 메인 프로젝트에서 Slidev 실행
   npx slidev --port 3040 theme-showcase.md
   ```

3. **자동 반영**
   - npm link로 연결되어 있어 CSS/Vue 변경시 자동 반영
   - 브라우저 새로고침으로 즉시 확인 가능

## 파일 구조

```
slidev-theme-one-purple-unicorn-pro/
├── index.ts           # 테마 설정
├── styles.css         # 엔트리 CSS
├── styles/
│   └── index.css      # 메인 스타일
├── layouts/           # Vue 레이아웃
│   ├── cover.vue      # 표지
│   ├── intro.vue      # 소개
│   ├── section.vue    # 섹션
│   └── end.vue        # 마지막
└── package.json       # 패키지 설정
```

## 주요 명령어

```bash
# 테마 링크 (연결)
npm run link

# 테마 언링크 (연결 해제)
npm run unlink

# 개발 모드
npm run dev

# 빌드
npm run build
```

## 스타일 가이드

### 색상 시스템
```css
--primary: #8b5cf6;        /* Purple */
--primary-light: #a78bfa;  /* Light Purple */
--primary-dark: #7c3aed;   /* Dark Purple */
--secondary: #6366f1;      /* Indigo */
--accent: #ec4899;         /* Pink */
```

### 레이아웃 스타일
- **cover**: 큰 제목, 그라디언트 배경
- **section**: 중간 크기, 섹션 구분
- **intro**: 소개 페이지
- **end**: 마지막 페이지

## 테스트

```bash
# 테마 쇼케이스 실행
npx slidev --port 3040 theme-showcase.md

# 실제 프레젠테이션 테스트
npx slidev --port 3040 [your-presentation].md
```

## 주의사항

1. **npm link 상태 유지**: 개발 중에는 항상 link 상태 유지
2. **브라우저 캐시**: 강제 새로고침 (Cmd+Shift+R) 필요할 수 있음
3. **Vue 파일 변경**: HMR이 작동하지 않을 경우 서버 재시작

## 배포 준비

```bash
# 링크 해제
npm run unlink

# npm 패키지로 발행 (필요시)
npm publish
```