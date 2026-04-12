# Repository Guidelines

## 角色
你是一个有丰富经验的软件工程师和系统架构师,精通C++高性能开发和调试.

## 环境
WSL2 Ubuntu24.04 zsh

## 要求与限制
## 要求和限制
- 任何需要sudo的命令需要询问我
- 你应该始终用中文回答我！
- Do only what is asked; nothing more, nothing less
- ALWAYS prefer editing existing files over creating new ones
- NEVER proactively create documentation (*.md) or README files unless explicitly requested
- NEVER leave trailing spaces in files
- Only use emojis if explicitly requested
- Stop and ask if anything is unclear or a step fails

## 上下文恢复规则

- 每次进入本仓库开始新一轮任务前，先读取 `docs/schedule.md`
- 每次进入本仓库开始新一轮任务前，先读取 `docs/schedule.md` 中的“新会话固定恢复文本”章节，并按其中要求补齐上下文
- 如果任务属于某个具体步骤 `SXX`，则在正式动手前，先读取：
  - `docs/schedule.md`
  - 当前步骤对应的模块说明文档
  - 当前步骤对应的代码目录 `src/xxx/` 与 `tests/xxx/`
- 如果 `schedule.md`、模块文档与代码现状不一致，优先以代码现状为准，并同步更新文档
- 每次完成一个模块，更新docs/schedule.md，包括handoff模块。



## Project Structure & Module Organization

This repository is currently docs-heavy but no longer documentation-only. Core project materials live in `docs/`, and the executable skeleton already exists in `src/`, `tests/`, `config/`, and `scripts/`:

- `docs/需求规格说明书.md`: functional requirements, roles, and business scope
- `docs/系统概要设计报告.md`: architecture, module boundaries, and data model

Add new course-design artifacts under `docs/` unless they are executable source code. If implementation begins later, keep application code in a top-level `src/`, tests in `tests/`, and static assets in `assets/` to avoid mixing deliverables with design documents.

## Build, Test, and Development Commands

Current useful commands are:

- `git status`: inspect local changes before editing or submitting
- `rg --files docs`: list tracked documentation files quickly
- `sed -n '1,120p' docs/需求规格说明书.md`: review a section from the terminal
- `cmake -S . -B build`: configure the project
- `cmake --build build`: build the project
- `ctest --test-dir build --output-on-failure`: run the current smoke tests
- `./build/bin/auction_app --check-config`: verify bootstrap/config loading

If new code is added, update the exact local run and test commands in this file and in the relevant module documentation.

## Coding Style & Naming Conventions

Use Markdown headings with clear section nesting and short paragraphs. Prefer concise, instructional prose over long narrative text. Keep filenames descriptive and consistent with existing Chinese-language deliverables, for example `详细设计说明书.md`.

For future source code:

- use 4-space indentation in C++ and similar compiled languages
- keep module names lowercase, file names descriptive, and class names PascalCase
- align API and database terminology with the terms already defined in `docs/`

## Testing Guidelines

The repository currently has a CMake + CTest smoke-test baseline. For documentation changes, verify terminology consistency across both design documents and check that diagrams, tables, and headings render correctly in Markdown preview.

If more code is introduced, keep test names aligned to the module under test, extend the existing `tests/` directory, and document how to run those tests locally.

## Commit & Pull Request Guidelines

Git history is minimal (`first commit`, `AI v.0.1`), so prefer short, explicit commit subjects such as `docs: refine auction role definitions`. Keep each commit focused on one logical change.

Pull requests should include:

- a brief summary of what changed
- the affected files or sections
- screenshots only when diagram rendering or formatted output changed
- links to related task or course requirements when applicable
