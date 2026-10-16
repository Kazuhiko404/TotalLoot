# Feature: Complete looting (animal looting migration + CheatLoot)

## Objective
1. TotalLoot becomes the single complete looting mod. It absorbs the animal looting from
   TotalPredator: the LOOT action for player animals, the AUTO LOOT toggle on a player animal, and
   the backpack-first transfer with stacking, money credit and stolen-flag policy. TotalPredator
   keeps only feeding.
2. A new `TotalLoot.ini` option `CheatLoot=0/1` controls Loot and Loot all:
   - `0` (default): the looter walks to each body (the target, or every body within `LootRadius`)
     and loots them one by one on arrival.
   - `1`: today's instant remote loot.

## Problem / why
- Loot and Loot all transfer items from any distance. That is a cheat.
- The 20 m radius is hardcoded in `loot_overlay.cpp`.
- TotalPredator duplicates the looting stack with a better transfer routine. With both mods
  installed, their buttons would collide on the same context menu.

## Scope
- In: config module and INI; transfer and target-predicate port; multi-actor order watch; walking
  loot state machine and session; dispatcher; animal Loot and AUTO LOOT; TotalPredator looting
  removal; docs.
- Out: theft odds; feeding behaviour; pick-pocket behaviour changes.

## Constraints
- Each concern lives in its own module (no patch-on-patch).
- SEH guards (`TL_TRY`/`TL_CATCH`) go around every engine or MyGUI pointer access.
- Behaviour the user already validated (pick-pocket, button placement, TotalPredator transfer
  rules) must not be dropped.
- Deploying must never overwrite a user-edited INI.
- Artifacts are in English. Toolset is v100 (VS2010 C++ subset: no `auto` lambdas tricks beyond
  what the existing code uses, no C++11 library features unless the existing code does).

## TDD
- Mode: on (global `Strict TDD Mode: enabled`).
- Runner: `MSBuild tests\totalloot_tests.vcxproj -p:Configuration=Release -p:Platform=x64`, then
  `tests\x64\Release\totalloot_tests.exe`.
- MSBuild path: `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe`.
- DLL build needs:
  - `KENSHILIB_DIR=F:\Proyectos\Kenshi-mods\KenshiLib_Examples_deps\KenshiLib`
  - `BOOST_INCLUDE_PATH=F:\Proyectos\Kenshi-mods\KenshiLib_Examples_deps\boost_1_60_0`

## Review
- RDD: off (global), so ordinary checks only.

## Delivery
- Branches:
  - TotalLoot: `feat/complete-looting` (off `feat/pick-pocket`).
  - TotalPredator: `feat/move-looting-to-totalloot` (off `master`).
- Strategy: ask-on-risk. No push and no merge; those are the user's call.

## Tasks
- [x] T1 Config module (`loot_config`), `TotalLoot.ini`, `package_deploy.py`, with parser tests.
- [x] T2 `loot_targets` and `loot_transfer` port from TotalPredator; `ExecuteLootSingle` uses it.
- [x] T3 Multi-actor `order_watch` (source-compatible), with pick-pocket tests green.
- [x] T4 `loot_walk_rules` pure queue state machine (RED/GREEN).
- [x] T5 `loot_walk_session` and `loot_dispatch`; overlay wired; CheatLoot=0 walking Loot and
      Loot all.
- [x] T6 Animal looting: Loot for animal looters, plus the AUTO LOOT toggle on player animals.
- [x] T7 TotalPredator: remove looting (buttons, auto-loot, transfer, `[Looting]` config, docs).
- [x] T8 Docs: README configuration and CheatLoot; QA checklist entries.

## Acceptance criteria
- `CheatLoot=0`:
  - Loot on a far body walks the looter there, then loots on arrival.
  - Loot all visits every body in `LootRadius`: the clicked one first, then nearest-first.
  - A player order cancels the walk.
- `CheatLoot=1`: Loot and Loot all are instant, as before.
- Player animals can Loot, and AUTO LOOT walks them through the bodies. It stops when the
  backpack is full, in combat, or when a nearby ally is attacked.
- TotalPredator shows no loot buttons, and feeding still works.
- The test suites pass, and both DLLs build.

## Route log
| Task | Route | Trigger evidence | Commit | Checks |
|---|---|---|---|---|
| T1 | delegated direct | writer, 2+ non-trivial files | 524b5ac | tests 4/4 suites, 276 checks pass (RED: stub parser, 20 failing checks; GREEN after implementation); DLL build ok |
| T2 | delegated direct | writer, 2+ non-trivial files | 0144ccd | tests 4/4 pass; DLL build ok (engine-bound, build-verified only) |
| T3 | delegated direct | writer, 2+ non-trivial files | bcf43a6 | tests 5/5 suites, 306 checks pass (RED: stub set, 16 failing checks; GREEN); DLL build ok |
| T4 | delegated direct | writer, 2+ non-trivial files | 27ec34b | tests 6/6 suites, 583 checks pass (RED: stub machine, 136 failing checks; GREEN first implementation); DLL build ok |
| T5 | delegated direct | writer, 2+ non-trivial files | f177010 | tests 7/7 suites, 606 checks pass (RED: refcount tests 3 failing, then dispatch-rules stub 5 failing; GREEN); DLL build ok (session, dispatch, tick hook are engine-bound: build-verified, in-game check pending) |
| T6 | delegated direct | writer, 2+ non-trivial files | a21c6e3 | tests 8/8 suites, 617 checks pass (RED: stub rules, 5 failing checks; GREEN); DLL build ok (button and overlay wiring are engine-bound: in-game check pending) |
| T7 | delegated direct | writer, 2+ non-trivial files (TotalPredator repo) | TotalPredator dd40f5f | Release x64 build ok before and after; +21/-1515 lines; no tests in TotalPredator |
| T8 | delegated direct | writer, 3 doc files | 372fd99 | docs only; README, QA checklist and feature doc re-read, no source touched |

## Progress
- 2026-10-03: plan approved, branches created.
- 2026-10-03: T1-T4 done and committed. T2 added `loot_inventory`, `loot_stacking` and `loot_money`
  (one concern each) beside `loot_targets` and `loot_transfer`. T3 extracted the pure
  `order_watch_set`. `order_watch` now takes the actor: `TakeForeignOrder(actor, out)`,
  `UnwatchOrdersOf(actor)`.

- 2026-10-03: T5 done. New modules, one concern each: `tick_dispatch` (the single mainLoop hook;
  pick pocket now registers its tick there instead of hooking, since two modules must not hook one
  address), `loot_move` (`IssueMoveTo`/`StopActor`), `loot_threats` (operational/combat/ally-attacked
  reads), `loot_dispatch_rules` (pure route and area queue, tested), `loot_walk_session`,
  `loot_dispatch`. `order_watch_set` is now ref-counted so pick pocket and a walker can share an
  actor. `loot_overlay` calls only the dispatcher.

- 2026-10-03: T6 done. `auto_loot_rules` (pure, tested) decides visibility and caption;
  `auto_loot_button` mirrors the pick-pocket button and joins the shared free-slot block in the
  overlay. Loot for animal looters needed no change: `IsTargetLootable` and `SelectedCharacter`
  never excluded animals.

- 2026-10-03: T8 done. README gained Configuración, CheatLoot, Looteo con animales and Transferencia
  sections; QA checklist gained config/deploy, walking loot, instant loot, animal looting and layout
  checks. Note: Loot all under CheatLoot=1 now also uses `LootRadius` (no longer a fixed 20 m).

- 2026-10-03: T7 done in TotalPredator (dd40f5f): looting removed, feeding kept, build ok.

## Next step
- In-game QA (docs/QA-CHECKLIST.md); push/merge of both branches is the user's call.
