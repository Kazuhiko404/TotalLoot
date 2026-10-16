# Feature: Pick pocket

## Objective
While the player's selected character is in stealth mode, right-clicking a conscious NPC shows a
"Pick pocket" button beside the native context menu. Clicking it opens the NPC's inventory in the
native loot window so the player can steal items. Theft odds, detection, crime and Thievery XP are
the engine's own (same path as stealing from a sleeping NPC).

All TotalLoot context-menu buttons auto-place LEFT or RIGHT of the menu wherever there is free
space, never overlapping buttons from other mods (TotalSparring, TotalRecruit, TotalPredator, ...).

## Problem / why
The reference mod StandingPickpocket (workshop 3810081680) is buggy: its click handler listens to
the mouse *release* with the wrong button id (right instead of left) and the menu closes before the
release arrives. TotalLoot's own buttons are pinned left blindly and no mod avoids overlap today
(TotalRecruit and TotalSparring already overlap each other on the right).

## Verified engine facts (research 2026-10-01)
- `ForgottenGUI::_showTradeWindow(RootObject* actor, RootObject* target, TW_LOOTING=2)` opens the
  native loot window immediately; it does no range/consciousness checks.
- Moving an item out of a non-player inventory runs the engine transfer path: `isStealing` ->
  `isWithinRangeToTrade` (horizontal dist^2 < 3600, same floor, same building) ->
  `BountyManager::setCrime(CRIME_STEALING)` for conscious victims -> `Character::ImStealingDoYouNotice`
  (`getStealingSuccessChance`, STAT_THIEVING vs nearest observer) -> `THIEF_DETECTED` / XP.
- Stealth: `Character::isStealthMode()`, `setStealthMode(bool)`.
- Approach: `Character::addOrder(building, MOVE_CUS_ORDERED=29, nullptr, false, true, pos)`.
- Close: `ForgottenGUI::getInventoryWindow(hand)`, `closeInventory(hand)`.
- Unverified: whether a conscious victim blocks item movement in the live game (in-game check).

## Scope
- In: Pick pocket button + flow (eligibility, approach, open native window, auto-close on
  range/stealth loss); shared free-slot placement for all TotalLoot buttons.
- Out: reimplementing theft odds; changing other mods.

## Constraints
- Each concern in its own module (no patch-on-patch): slot placement, pickpocket rules,
  pickpocket session/flow, overlay wiring.
- SEH guards (`TL_TRY`/`TL_CATCH`) around every engine/MyGUI pointer access.
- Artifacts in English.

## TDD
- Mode: ON. Source: global CLAUDE.md "Strict TDD Mode: enabled".
- Runner: pure-logic test exe `tests/totalloot_tests.vcxproj` (MSBuild Release x64, run exe),
  modelled on `TotalSparring/tests/duel_tests`. Engine-bound code is verified by build + in-game QA.

## Delivery
- Strategy: ask-on-risk. Branch `feat/pick-pocket` (baseline commit 9302772 on master).

## Tasks
- [x] T1 Free-slot placement module (`menu_slot`): pure geometry resolver + MyGUI obstacle scan;
      Loot/Loot all migrated to it. Route: delegated writer (2+ non-trivial files).
- [x] T2 Pick pocket rules (pure eligibility/range logic, tested) + session/flow + button wiring.
      Route: delegated writer (same writer, sequential).
- [x] T3 Build, package to ModPackage, README update, in-game QA checklist.
- [x] T4 Shared click input module (`context_button_input` + pure `click_gesture`): press AND release,
      one action per gesture, used by Loot, Loot all and Pick pocket. Route: single writer (same session).
- [x] T5 Shared look (`button_style`): TotalSparring's size/skin/colours/font for all TotalLoot buttons;
      `menu_slot` pinned with a 160x44 test.
- [x] T6 Replace the false "new order" detection: `order_watch` hooks `Character::addOrder`; state machine
      takes `externalOrder`; session logging for the next in-game test.

## Acceptance criteria
- Button appears only when: selected char is a player character in stealth, not in combat, not
  in dialogue; target is a valid, conscious, non-player, non-animal, non-dead, non-carried NPC.
- Click (left mouse, press or release; or hold right, slide, release over the button) works every time and
  acts once; menu closes; character walks to the target if
  needed; native loot window opens; items taken trigger engine theft checks.
- Window auto-closes if stealth drops, range is lost, actor/target invalid, or player reorders.
- No TotalLoot button overlaps any visible foreign widget beside the menu; prefers left, then
  right, then stacks below on either side; clamps to screen.

## Progress / evidence
- T1 done (route: delegated writer, trigger: 2+ non-trivial files).
  - RED: `MSBuild tests/totalloot_tests.vcxproj` + run exe with stub `ResolveSlot` -> segfault
    (exit 139); stub `IsPlausibleObstacle` -> `0/1 suites passed, 3/57 checks failed`.
  - GREEN: same command -> `1/1 suites passed, 0/57 checks failed`, exit 0.
  - `MSBuild TotalLoot.vcxproj Release x64` -> 0 errors (TotalLoot.dll linked; `Gui::getEnumerator` resolves).
  - Modules: `menu_slot` (pure), `menu_obstacles` (MyGUI scan), `button_block` (apply), loot_overlay migrated.
  - Decision: "stack below" slides the block down past whatever it touches (iterative), which is a
    superset of "below the lowest obstacle in the column" but does not jump over unrelated far obstacles.
  - Commit: see git log (T1 commit).

- T2 done (route: delegated writer; trigger: 2+ non-trivial files).
  - Pure rules commit ba0b4ce: RED `48/133 checks failed` (stubs) -> GREEN `2/2 suites passed, 0/133`.
  - Follow-up RED (new test UnobservedOrderNeverCountsAsReorder): `1/2 suites passed, 1/134 checks failed`
    -> GREEN `2/2 suites passed, 0/134 checks failed`.
  - `MSBuild TotalLoot.vcxproj Release x64` -> 0 errors, 0 warnings from TotalLoot sources.
  - Modules: pickpocket_rules (pure), pickpocket_reads / pickpocket_actions (SEH engine access),
    pickpocket_session (state + GameWorld::_NV_mainLoop_GPUSensitiveStuff hook), pickpocket_button
    (TL_PickPocketButton, left-click on press only, no release subscription), target_character
    (shared cast + selected character), loot_overlay assembles one block (Loot, Loot all, Pick pocket).
  - Decision: `ForgottenGUI::_showTradeWindow` is declared public in KenshiLib's header (the "private"
    is only the RVA-dump comment), so it is called directly; no macro or member-pointer trick.
  - Decision: range uses horizontal dist^2 only (engine rule); vertical offset is not part of it.
  - Decision: "player gave another order" is trusted only after the engine has reported our move order
    at least once (orderSeen), so an unreliable `hasPlayerOrder(MOVE_CUS_ORDERED)` cannot cancel a
    healthy approach.

- T3 done: Release build exit 0, `TotalLoot.dll` copied to `ModPackage/TotalLoot/` (git-ignored artifact),
  tests exe `2/2 suites passed, 0/134 checks failed`; README (Spanish, consistent) and
  `docs/QA-CHECKLIST.md` written.
- Commits: T1 6c96c72, T2 rules ba0b4ce, T2 session/button 2572821, T3 docs (see git log).

- T4/T5 done (commit 3f933f7, one commit because the vcxproj lists both modules).
  - RED: click_gesture stubs (`Classify` -> STRAY) -> `2/3 suites passed, 8/146 checks failed`;
    GREEN: `3/3 suites passed, 0/146 checks failed`. menu_slot 160x44 pin test: `0/154 failed` (geometry is
    generic, so it passed on first run; it documents the new size).
  - TotalSparring reference: `src/spar/ui/spar_overlay.cpp:307-417` (handler bound to pressed AND released,
    target taken and cleared by the first event; `:464-485` binding + style; `:27-37` size consts) and
    `src/shared/ui/paired_click_rules.h` (act / echo / lost). No InputManager hook exists there.
  - Style copied: skin `Kenshi_Button1` (fallback `Button`), layer `Popup`, 160 min width (capped by menu
    width), 44 high, gap 2/2, font height 18, text colour amber (236,184,96), text shadow black, widget
    colour brass (214,176,116); hover: colour amber + text colour bgDark (30,27,24), leave restores.
    No custom skin/resource file exists for these buttons in TotalSparring, so nothing to ship.
- T6 done.
  - Root cause: `hasPlayerOrder(MOVE_CUS_ORDERED)` reads the OrdersReceiver `orders` queue (ActionDeque); the AI
    consumes the queued order into `currentGoal` within its first think, so the query flips true -> false while
    the actor is still walking. Our old rule read that as "player replaced our order" once the 0.75 s grace
    passed: every cancel in RE_Kenshi_log.txt is 0.78-1.33 s after the start (grace 0.75 s + one tick).
    Sessions at 1404.012 / 1441.265 did not cancel: target already within 40 units, the window opened on the
    first tick (WASDCombat `loot_ui_open_suspend_vmode` 0.46 / 0.46 s later), so the order path never ran.
  - Fix: `order_watch` hooks `Character::addOrder` (non-virtual, RVA 0x5D1950); our own call is bracketed by
    `BeginOwnOrder/EndOwnOrder`; any other addOrder on the actor sets a pending flag read once per tick.
    `hasMoveOrder`, `orderSeen` and the grace constant were removed from the pure machine.
  - RED: new rules tests with the old logic -> `2/3 suites passed, 3/197 checks failed`; GREEN after the
    change `0/197`; `ReasonName` RED `37/234` -> GREEN `0/234`.
  - Decision: selecting a different character is NOT treated as an order for the actor (it issues nothing to
    it); right-click-moving or any other order to the actor cancels.

## Next step
- In-game QA via `docs/QA-CHECKLIST.md` (engine behaviour is unverified until run in the game).
