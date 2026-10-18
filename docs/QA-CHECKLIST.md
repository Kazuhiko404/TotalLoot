# TotalLoot in-game QA checklist

Run in Kenshi 1.0.65 with RE_Kenshi and `ModPackage/TotalLoot` installed. Tick each
item only after observing it. Record failures with the log excerpt from `TotalLoot` logs.

## Pick pocket: visibility
- [ ] Selected character NOT in stealth: right-click a conscious NPC -> no "Pick pocket" button.
- [ ] Selected character in stealth: right-click a conscious, standing NPC -> "Pick pocket" appears.
- [ ] Button never appears on: a player squad member, an animal, a dead/KO'd/downed NPC, a carried NPC.
- [ ] Button does not appear while in combat, in dialogue, or with a modal message open.
- [ ] Toggle stealth with the menu open: no crash (button state is decided when the menu opens).

## Pick pocket: click and approach
- [ ] Left click works the first time, every time (also on repeated attempts), and acts once (log shows ONE session start).
- [ ] Hold the right mouse button on the NPC to open the menu, slide onto the button and RELEASE: the action fires (same as the native menu entries and TotalSparring).
- [ ] Release the right button on the world / on a native entry instead: no TotalLoot action fires.
- [ ] Middle click on the button does nothing.
- [ ] Same checks for Loot and Loot all (a click never loots twice).
- [ ] The native menu closes and the button does not flicker back for a frame.
- [ ] Target far away: the character walks to the target, stops close, and the native loot window opens.
- [ ] Target already close (< 40 units): the window opens immediately.
- [ ] Target walking away: the character follows and re-paths (about once per second) and still opens the window.
- [ ] Target unreachable (blocked path): after about 15 s without progress the approach cancels with a message.
- [ ] Right-click-moving the character while it approaches cancels pick pocket (message "new order given") and does NOT fight the move.
- [ ] Opening the menu and starting pick pocket never cancels itself: no "new order given" without an order from you.
- [ ] Dropping out of stealth while approaching cancels with a message.
- [ ] Taking damage / combat starting while approaching cancels with a message.
- [ ] Approach never takes more than 60 s.
- [ ] Out-of-range start: the character walks all the way (the old false cancel ~0.8 s after the click is gone).
- [ ] Log (RE_Kenshi_log.txt) shows, per attempt: "session started: distance ..., out of range / already in range",
      "approach move order #1 issued", "loot window opened (...) getInventoryWindow visible=...", and on any end a line
      with `reason=...` (for a foreign order also `type=` / `shift` / `clear` and seconds after start).

## Pick pocket: theft and engine behaviour
- [ ] Moving an item out of a CONSCIOUS victim's inventory actually works (unverified engine assumption).
- [ ] Taking an item with low Thievery can trigger detection / crime (THIEF_DETECTED, bounty) like stealing from a sleeper.
- [ ] Successful theft grants Thievery XP.
- [ ] Items can be taken into the thief's inventory normally (no duplication, no loss).

## Pick pocket: auto-close
- [ ] With the window open, leaving stealth closes the window and shows a message.
- [ ] Walking away from the victim (beyond about 60 units) or the victim walking away closes the window.
- [ ] Victim goes to another floor / enters a building: window closes.
- [ ] Victim dies, gets knocked out, or despawns: window closes without a crash.
- [ ] Closing the window by hand (Esc / close button) ends the session cleanly; a new attempt works.
- [ ] Save and load with the window open / mid-approach: no crash, no stuck session.

## Button look
- [ ] Loot, Loot all and Pick pocket are 160x44, 2 px apart, brass tint with amber 18 pt text and a dark shadow.
- [ ] Hovering a button turns it amber with dark text; leaving restores brass/amber (identical to TotalSparring's buttons).
- [ ] Side by side with TotalSparring's buttons the two mods' buttons are indistinguishable except for the caption.
- [ ] TotalLoot works without TotalSparring installed (the skin is the stock Kenshi_Button1; no extra resource file).

## Button placement
- [ ] TotalLoot buttons sit LEFT of the native menu, top-aligned, when free.
- [ ] Menu near the left screen edge: buttons go to the RIGHT.
- [ ] Menu near the right screen edge: buttons never leave the screen (the old right-edge bug is fixed).
- [ ] With TotalSparring buttons visible (160x44, right of the menu): no overlap with TotalLoot buttons.
- [ ] With TotalRecruit button visible: no overlap.
- [ ] With TotalPredator buttons (Eat / Loot / Auto-loot, normally below the menu): no overlap.
- [ ] All of the above mods active on one target: block stacks below the foreign buttons or switches side; no overlap.
- [ ] No flicker between left and right while the menu stays open (cross-mod hook order).
- [ ] Screen corners and very small menus: block stays fully on-screen.

## Regression: Loot / Loot all
- [ ] On a KO'd / dead / downed NPC: "Loot" and "Loot all" appear exactly as before; "Pick pocket" does not.
- [ ] On a lootable container: Loot / Loot all appear; Pick pocket does not.
- [ ] On a conscious squadmate or conscious NPC with no stealth: no TotalLoot button.
- [ ] Loot all area loot (`LootRadius`, 20 m by default) still transfers items and stops when the inventory is full.
- [ ] Conscious NPC in stealth: only "Pick pocket" shows (no Loot / Loot all), placed in the same slot logic.

## Configuration and deploy (TotalLoot.ini)
- [ ] First deploy installs `TotalLoot.ini` next to `TotalLoot.dll`; the log shows "Config loaded from ..." with the active values.
- [ ] Edit a value (e.g. `LootRadius=40`), run `python package_deploy.py` again: the script prints "Kept existing config" and the edit survives; after restarting Kenshi the log shows the new value.
- [ ] Delete `TotalLoot.ini`: the log reports defaults, nothing crashes.
- [ ] Invalid values (`LootRadius=abc`, `LootRadius=-5`) fall back to the default; `LootRadius=9999` is clamped to 500.

## Walking loot (CheatLoot=0, default)
- [ ] Loot on a far body: the character walks to it, loots on arrival, backpack is filled first, then the main inventory.
- [ ] Loot all with 3 bodies in range: the clicked body is looted first, then the others nearest-first.
- [ ] Bodies of player characters are skipped by Loot all.
- [ ] Any player order (right-click move) while walking cancels the session and does not fight the move.
- [ ] Combat starting while walking cancels it (`cancel_on_combat=1`); with `cancel_on_combat=0` it keeps going.
- [ ] A nearby ally attacked cancels it (`cancel_when_nearby_ally_attacked=1`); with `0` it keeps going.
- [ ] Inventory full: Loot all stops after 3 bodies in a row that yield nothing, leftovers stay on the bodies.
- [ ] Unreachable body: gives up after about 15 s without progress; a single body never takes more than 60 s.
- [ ] Container loot (CheatLoot=0): the character walks to the container and loots it on arrival; a player-faction container gets no Loot button.
- [ ] Money items are credited to the wallet (no inventory slot), items stack with existing stacks.
- [ ] `clear_stolen_flag=1`: looted items from a living victim carry no stolen mark; `0`: native behaviour.

## Instant loot (CheatLoot=1)
- [ ] Loot and Loot all transfer instantly from any distance, exactly as before; Loot all honours `LootRadius`.

## Animal looting
- [ ] Select a player animal, right-click a body: Loot / Loot all appear and work (walk when CheatLoot=0).
- [ ] "Auto loot" appears only when right-clicking your own LIVING animal; not on other animals, dead/KO'd animals, humans or squadmates.
- [ ] Pressing it starts the walk through nearby bodies (within `LootRadius` of the animal); the caption toggles to "Stop auto loot" and pressing it stops.
- [ ] Auto loot walks even with CheatLoot=1.
- [ ] Auto loot stops when the animal's backpack is full, in combat, or when a nearby ally is attacked.

## Button layout with the new buttons
- [ ] Loot, Loot all, Pick pocket and Auto loot never overlap each other when several show at once.
- [ ] With TotalSparring, TotalRecruit and TotalPredator buttons visible: no overlap with any TotalLoot button.
- [ ] Pick pocket still works on a conscious NPC while another character is walk-looting.
- [ ] With TotalPredator installed (looting removed) there are no duplicate LOOT / AUTO LOOT buttons; its feeding still works.

## Shutdown
- [ ] Quit to the main menu and reload a save: buttons recreate, no duplicated widgets, no crash.

## BetterLooting merge: filters (defaults)
- [ ] With the shipped INI defaults, Loot, Loot all and Auto loot on a KO'd and on a dead body take everything, as before.
- [ ] Set `Food=0`: Loot leaves food in the body; other categories still transfer.
- [ ] Set `MinValue=500`: cheap items stay on the body; valuable items transfer.
- [ ] `AllowStolen=0`: stolen body loot is skipped; `AllowStolen=1`: it is taken.
- [ ] Money is always credited, even with `MinValue` high and every category off.
- [ ] Walking Loot all (`CheatLoot=0`) over bodies whose items are all filtered: it moves on to the next body and does not stop early.
- [ ] `PreferBackpack=1` fills the backpack first; `PreferBackpack=0` fills the main inventory first.
- [ ] `BestItemsFirst=1`: the most valuable items transfer first when space runs out.
- [ ] Animal Auto loot applies the same filters.

## BetterLooting merge: hotkey and storage
- [ ] Insert with the player's inventory open: radial loot inside `LootRadius` starts instantly.
- [ ] Insert, with a target's inventory being opened: the target is looted first, then the radius.
- [ ] Insert uses the player's own storage (chest) as a destination.
- [ ] Changing `ActivationVirtualKey` in the window and saving takes effect without restart.
- [ ] DEPOSITAR TODO (Store all) in the settings window moves items into player storage.
- [ ] Items looted this way are not moved back out by the game's auto-deposit.

## BetterLooting merge: settings window
- [ ] Shift+P opens the window; Shift+P again closes it.
- [ ] Layout: window is 812 px tall, no overlapping controls, footer buttons fully visible.
- [ ] ES/EN switch in the header changes every label; after Save, `[UI] Language` matches the selected language.
- [ ] Toggle `CheatLoot` to 1 and press Save (no restart): a Loot now runs instantly from a distance. Toggle back to 0, Save: Loot walks again.
- [ ] `ArriveDistance`, `clear_stolen_flag`, `cancel_on_combat` and `cancel_when_nearby_ally_attacked` saved from the window take effect without restart.
- [ ] Shared radius control: Save clamps `LootRadius` to 1-100 m.
- [ ] DEFAULTS restores the default values and saves them immediately.
- [ ] Pick pocket and button placement still work after the merge.
