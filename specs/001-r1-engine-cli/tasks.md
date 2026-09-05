---
description: "Task list for R1 Engine and CLI"
---

# Tasks: R1 Engine and CLI

**Input**: Design documents from `/specs/001-r1-engine-cli/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/

**Tests**: Included (TDD). Spec SC-006, plan Qt Test, constitution/discovery require math tests and a DisplayBackend mock. Write tests first; they MUST fail before implementation.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- Single CMake project: `src/`, `tests/` at repository root (plan.md)

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic structure

- [ ] T001 Create `src/core/`, `src/backend/`, `src/cli/`, and `tests/` per `specs/001-r1-engine-cli/plan.md`
- [ ] T002 Write `CMakeLists.txt` for C++20, `find_package(Qt6 REQUIRED COMPONENTS Core Test)`, `find_package(KF6Screen REQUIRED)`, `kscaling` executable, CTest
- [ ] T003 [P] Add `.clang-format` at repository root for C++

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [ ] T004 Write failing Qt Test Fixture A goldens and Fixture B perfect in `tests/ResolutionMathTest.cpp`
- [ ] T005 [P] Define closed preset table (`perfect` 1.25, `max-space` 1.00, `comfort` 1.60, `large` 1.778) in `src/core/Preset.h`
- [ ] T006 Implement `snap8` ModePlan (aspect-preserving, canvas 2×, Hz heuristic) in `src/core/ResolutionMath.h` and `src/core/ResolutionMath.cpp` until T004 passes
- [ ] T007 [P] Define `DisplayBackend` types and `list` / `applyCustom` / `revert` in `src/backend/DisplayBackend.h` per `specs/001-r1-engine-cli/contracts/display-backend.md`
- [ ] T008 [P] Define `SavedProfile` fields in `src/core/Profile.h` per `specs/001-r1-engine-cli/data-model.md`
- [ ] T009 Implement `profiles.json` load/save under `QStandardPaths::AppConfigLocation` in `src/core/Settings.h` and `src/core/Settings.cpp`
- [ ] T010 [P] Implement mock that assigns mode ids only after simulated phase A in `src/backend/MockDisplayBackend.h` and `src/backend/MockDisplayBackend.cpp`
- [ ] T011 Define parse result and exit codes 0–4 in `src/cli/Cli.h` per `specs/001-r1-engine-cli/contracts/cli.md`
- [ ] T012 Stub `src/main.cpp` to dispatch CLI (Qt Core only; MUST NOT use Widgets or `QProcess`)

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Apply a looks-like preset (Priority: P1) 🎯 MVP

**Goal**: `--apply <preset> [--output]` registers then switches a 2× canvas at scale 2.00 and persists the profile.

**Independent Test**: `ctest` apply-orchestrator tests pass; on DP-3 `kscaling --apply perfect` looks like 2048×1152 / canvas 4096×2304 / scale 2.00.

### Tests for User Story 1

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [ ] T013 [P] [US1] Write failing two-phase apply tests (reuse by size+`qRound(hz)`, native=`preferred`, failed switch restores prior scale, no pre-phase-A id, incapable CustomModes → no mode/scale change and exit 1, reject does not retry Hz) in `tests/ApplyOrchestratorTest.cpp`

### Implementation for User Story 1

- [ ] T014 [US1] Implement apply orchestration (write-once `originalScale`, persist only on success) in `src/core/ApplyService.h` and `src/core/ApplyService.cpp`
- [ ] T015 [US1] Implement libkscreen two-phase `applyCustom` (`GetConfigOperation` / `SetConfigOperation` / `exec()`, abort if `!CustomModes`, full `setCustomModes` list, no guessed ids, no Hz retry on reject) in `src/backend/LibKScreenBackend.h` and `src/backend/LibKScreenBackend.cpp`
- [ ] T016 [US1] Implement `--apply` / `--output` parsing and exit 3/4 rules in `src/cli/Cli.cpp`
- [ ] T017 [US1] Wire `src/main.cpp` `--apply` to `ApplyService` + `LibKScreenBackend` and add SPDX `GPL-3.0-or-later` headers on new `src/` files

**Checkpoint**: User Story 1 fully functional and testable independently

---

## Phase 4: User Story 2 - List outputs and computed modes (Priority: P2)

**Goal**: `--list` prints connector names, capability, native/current, scale, and four preset rows.

**Independent Test**: Mock `--list` shows `DP-3` and Fixture A golden pairs; incapable outputs marked, never identified by numeric id.

### Tests for User Story 2

- [ ] T018 [P] [US2] Write failing `--list` column and Fixture A preset-row tests in `tests/ListTest.cpp`

### Implementation for User Story 2

- [ ] T019 [US2] Implement `list()` on `LibKScreenBackend` in `src/backend/LibKScreenBackend.cpp` using `Output::name()` and `preferredMode()` as native
- [ ] T020 [US2] Implement `--list` printer in `src/cli/Cli.cpp` per `specs/001-r1-engine-cli/contracts/cli.md`

**Checkpoint**: User Stories 1 AND 2 work independently

---

## Phase 5: User Story 3 - Revert to the preferred panel mode (Priority: P3)

**Goal**: `--revert [--output]` restores preferred mode and originalScale; does not clear saved preset.

**Independent Test**: Apply `perfect` then `comfort`, `--revert` returns preferred mode + first-apply original scale; `profiles.json` still has last preset.

### Tests for User Story 3

- [ ] T021 [P] [US3] Write failing revert tests (originalScale write-once, profile file unchanged) in `tests/RevertTest.cpp`

### Implementation for User Story 3

- [ ] T022 [US3] Implement `revert` on `LibKScreenBackend` in `src/backend/LibKScreenBackend.cpp` (`preferredModeId` + optional originalScale)
- [ ] T023 [US3] Implement `--revert [--output]` in `src/cli/Cli.cpp` calling `ApplyService` without clearing `SavedProfile`

**Checkpoint**: User Stories 1–3 work independently

---

## Phase 6: User Story 4 - Restore the last successful profile (Priority: P4)

**Goal**: `--apply-saved` re-applies last successful profiles on connected connectors.

**Independent Test**: Apply `perfect`, `--revert`, `--apply-saved` restores `perfect`; disconnected connectors skipped; empty file exit 0.

### Tests for User Story 4

- [ ] T024 [P] [US4] Write failing `--apply-saved` tests (after revert, skip disconnected, empty → 0) in `tests/ApplySavedTest.cpp`

### Implementation for User Story 4

- [ ] T025 [US4] Implement apply-saved loop (mode+hz first, recompute from preset if missing) in `src/core/ApplyService.cpp`
- [ ] T026 [US4] Implement `--apply-saved` in `src/cli/Cli.cpp` (reject `--output` with exit 1)

**Checkpoint**: All user stories independently functional

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [ ] T027 [P] Add `--help` text in `src/cli/Cli.cpp`
- [ ] T028 Add SPDX `GPL-3.0-or-later` to every file under `src/` and `tests/`
- [ ] T029 Reject unknown preset ids with exit 1 in `src/cli/Cli.cpp`
- [ ] T030 Confirm `src/` contains no `QProcess` and no `kscreen-doctor` runtime spawn
- [ ] T031 Run `ctest --test-dir build --output-on-failure` per `specs/001-r1-engine-cli/quickstart.md`
- [ ] T032 [P] Write `README.md` with build/run pointers to `specs/001-r1-engine-cli/quickstart.md`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
  - Sequential in priority order is the intended path (shared `Cli.cpp` / `LibKScreenBackend.cpp`)
  - Parallel only if those files are split first
- **Polish (Phase 7)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: After Foundational — MVP apply path
- **User Story 2 (P2)**: After Foundational — can start after US1 if sharing `LibKScreenBackend.cpp` / `Cli.cpp`
- **User Story 3 (P3)**: After Foundational — needs Settings originalScale from US1 persist to be meaningful on disk
- **User Story 4 (P4)**: After US1 apply + persist; uses same two-phase apply

### Within Each User Story

- Tests MUST be written and FAIL before implementation
- Core (`ApplyService`) before libkscreen backend
- Backend before CLI wiring
- Story complete before next priority when sharing files

### Parallel Opportunities

- T003 with T001–T002 after dirs exist
- T005, T007, T008, T010, T011 after T002 (different files)
- T013 with no other US1 file
- T018, T021, T024 test files in parallel if stories are staffed separately
- T027 [P] with T032; T029 after T027 (same `src/cli/Cli.cpp`)
- T032 parallel with T028/T030/T031

---

## Parallel Example: User Story 1

```bash
# After Foundational:
Task: "Write failing two-phase apply tests in tests/ApplyOrchestratorTest.cpp"

# Then sequential (same apply path):
Task: "Implement apply orchestration in src/core/ApplyService.cpp"
Task: "Implement libkscreen two-phase applyCustom in src/backend/LibKScreenBackend.cpp"
Task: "Implement --apply / --output in src/cli/Cli.cpp"
Task: "Wire src/main.cpp --apply"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: `ctest` + optional live `kscaling --apply perfect` on DP-3
5. Demo if ready

### Incremental Delivery

1. Setup + Foundational → math + mock + settings ready
2. US1 apply → MVP
3. US2 list
4. US3 revert
5. US4 apply-saved
6. Polish (help, SPDX, no-doctor, README)

### Parallel Team Strategy

One implementer recommended (shared CLI/backend files). If two: A does US1 apply core, B does math/list tests only after T007/T010.

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to spec user stories US1–US4
- Verify tests fail before implementing
- Reject any task that shells out to `kscreen-doctor` or guesses mode ids before reload
- Live DP-3 Hz retries are forbidden; paste `errorString()` into discovery instead
- Next after tasks: `/speckit.analyze` (optional) then Superpowers `writing-plans` + TDD execute — not `/speckit.implement` on the same slice
