
Main features
=============
Thinker has several advanced planning routines that enhance the base game AI to manage the complexities of colony building in Alpha Centauri. In the original game, many of the AI aspects were left underdeveloped while the computer factions struggled to deal with various punitive penalties. This left the single player experience severely lacking, since the AI would have no counter to many simple strategies.

As a general principle, Thinker uses the same production bonuses than what the vanilla difficulty levels would grant the AI normally. There should no extra resources received by the AI unless this is specified in the configuration file. Thinker should not change the automated behaviour of units or governors in player-controlled factions. The mod also will not attempt to remake the lore, reorder the tech tree, or adjust most vanilla game design decisions, since the main goal is to make the AI play better given any game config options.

Thinker does not have any special save game format, so it's possible to open an old save and have the factions switch to the new AI and vice-versa. None of the `thinker.ini` config options are preserved in the save game either, but the units or resources spawned at the game start will remain.

Note that in `thinker.ini` config file, for binary settings **only zero values are treated as disabled**, any non-zero value usually enables the option. Whenever Thinker options are changed in the GUI, they are saved to `Alpha Centauri.ini`. While the same settings can be set in thinker.ini, **the values in Alpha Centauri.ini take priority**.

For testing purposes it is also possible to run old/new AIs side-by-side by setting `factions_enabled=3` or similar. In that case, only the factions in the first 3 slots will use Thinker AI if they are not player-controlled. By default, `factions_enabled=7` setting enables Thinker AI for all computer factions. Regardless of options, Thinker also includes a custom crash handler that writes output to `debug.txt` in the game folder if the game happens to crash for any reason.

In addition to the binary patches listed separately in this file, items that are listed after "Game Engine Patches" in `thinker.ini` apply to all factions regardless of `factions_enabled` setting. Some patches such as `ignore_reactor_power` and unit repair rate changes are imported from [Will To Power Mod](https://github.com/tnevolin/thinker-doer/).

**For a mostly exhaustive list of all features provided by this mod, refer to both this file and `thinker.ini`.** Enabling Thinker AI will affect many AI behaviors such as described in the list below.

1. Thinker fully controls the movement of most units, combat and non-combat alike, to manage the base placement and production much more effectively than usual.
2. New combat routines for land-based units will attempt to counter-attack nearby enemy units more often. If the odds are good enough, hasty attacks are executed more often than usual. The AI will fight the native units more aggressively, and it will also try to heal its units at monoliths.
3. Thinker base production AI will decide every item that is produced in a base. The build order can differ substantially from anything the normal AIs might decide to produce, and the difference can be easily noted in the vastly increased quantity of formers and crawlers the AIs might have.
4. Social AI feature will decide the social engineering models the AI factions will choose. It will attempt to take into account the various cumulative/non-linear effects of the society models and any bonuses provided by the secret projects. The AI is now capable of pop-booming if enough growth is attainable, and it will also try to avoid pacifist drones by switching out of SE models with too many police penalties. All the SE model effects are moddable because the AI is not hardcoded in any particular choices. This feature is also capable of managing all the custom factions.
5. Tech balance will assign extra importance on some specific techs when selecting what to research: requirements for formers, crawlers, recycling tanks, children's creches, and the 3 technologies to allow the production of more than 2 resources per square. If these items don't have any requirements, or the techs are not available to research, the tech progression is unaffacted by this feature. It will also not affect player faction research in any way.
6. Hurry items feature is able to use AI energy reserves to occasionally hurry base production. Items that can be hurried include most basic facilities, formers, and (only rarely) combat units. The AI will never rush secret projects in this way, but sometimes they can be completed quickly with overflow minerals.
7. `design_units` feature will introduce custom probe teams, armored crawlers, gravship formers, and AAA garrison prototypes for the computer factions.
8. `auto_relocate_hq` feature imports a game mechanic from Civilization 3 where lost/captured headquarters are automatically moved to another suitable base. This ensures the AI factions will not struggle without active headquarters.
9. AI mineral/nutrient production cost factors for each difficulty level can be changed from the `cost_factor` setting. Does not affect other difficulty related modifiers.
10. Content (non-drone) base population for each difficulty level can be adjusted from `content_pop_player` and `content_pop_computer` variables. By default these have the same values than vanilla game mechanics.


User interface additions
========================
Thinker's in-game menu can be opened by pressing `ALT+T`. It shows the current version and build date, and provides statistics about the factions if a game is loaded. In addition, some Thinker config options can be adjusted from its sub menu.

* World map labels feature draws a white marker around the population label of HQ bases to identify them more easily. Bases in golden age are also highlighted with a yellow label. Bases that have Flechette Defense System or Geo Survey Pods are highlighted with a blue label. If a base might riot on the next turn due to population increase or already has too many drones, it will be highlighted with a red label.
* Whenever the player instructs a former build an improvement that replaces any other item in the tile, the game will display a warning dialog. This dialog can be skipped by toggling the Former Warnings option here.
* When building or capturing a new base, the mod will automatically copy the saved build queue from **Template 1** to the new base. At maximum 8 items can be saved to the template, and the first item from the template will be automatically moved to current production choice when a new base is built or captured. Only the template saved to the first slot is checked, any other saved templates are ignored.
* To save the current queue to template, open a base and **right click** on the queue and select **Save current list to template**.
* This feature works in conjunction with the simple hurry cost option so that it's easy to start hurrying base production on the first turn without worrying about double cost mineral thresholds.


Map generator options
=====================
Using the default settings, the game's random map generator produces bland-looking maps that also have way too many small islands that make the navigation much harder for the AI. Thinker's alphax.txt modifies the WorldBuilder to produce bigger continents, less tiny islands, more rivers and ocean shelf on random maps. All landmarks that are placed on random maps can now be configured from `thinker.ini`. Nessus Canyon is also available but disabled by default.

As an optional feature, Thinker includes a rewritten map generator that replaces the old WorldBuilder. This can be toggled with `new_world_builder` config option or from Thinker's menu option. This feature supports all the world settings chosen from the game menus such as map size, ocean coverage, erosion, natives and rainfall level.

Ocean coverage as a percentage for each menu option is provided as a separate option `world_sea_levels`. Changing this variable has a huge impact on the generated maps. Lower numbers are almost always pangaea maps while higher numbers tend towards archipelago layouts.

This new generator algorithm is entirely different from the game vanilla version, so it does not parse some of the variables specified in `alphax.txt`. Any WorldBuilder variable names starting with Land, Continent, Hills or Plateau are not currently used by the generator. The other options listed below in this document are supported regardless if the `new_world_builder` is enabled or not.

Thinker's `faction_placement` algorithm tries to balance faction starting locations more evenly across the whole map area while avoiding unusable spawns on tiny islands. The selection also takes into account land quality near the spawn. The effect is most noticeable on Huge map sizes.

When `nutrient_bonus` setting is enabled, the placement algorithm tries to ensure each spawn location has at least two nutrient bonus resources for land-based factions. The placement also strongly favors spawns on river tiles for easier movement in the early game.

Another optional setting `rare_supply_pods` is not dependent on faction placement, instead it affects the whole map by reducing the frequency of random supply pods significantly. Thematically it never made much sense that the supply pods would be excessively abundant across the whole planet surface, while the Unity spaceship would supposedly only have limited space for extra supplies.


Improved combat mechanics
=========================
In Alpha Centauri, Fusion reactor technology was extremely important for military purposes as it cheapens the reactor cost almost by half and doubles the unit's combat strength. This made the tech extremely unbalanced for most purposes, and there's no good way around this if the tech tree is to have any advanced reactors at all. By default Thinker's `ignore_reactor_power` option keeps all reactors available, but ignores their power for combat calculation purposes. More advanced reactors still provide their usual cost discounts and also planet busters are unaffacted by this feature.

Another notable change is the introduction of reduced unit healing rates. In the vanilla game, units were often able to fully heal in a single turn inside a base but this is no longer the case. Prolonged offensives are no longer trivially easy against opponents since units may have to stop healing for multiple turns. The healing rate can be significantly increased by building Command Centers and similar facilities though.

Since v2.3 Thinker also controls the movement of nearly all AI combat units, not just non-combat units like formers or colony pods. AI code for land, sea and air units has been almost entirely rewritten. The only major exception here are any units with the missile chassis which are still handled by the default AI. Note that the interceptor behaviour for AI air units is not implemented, so they will not activate needlejets to intercept enemy units during the other factions turn.

Another novel addition has been naval invasions executed by the AI. This has been traditionally a weak spot for many AIs since they lack the coordination between many units to pull off such strategies. Thinker however is capable of gathering an invasion fleet with many transports and using other warships as cover to move them to a landing zone. Warships will also use artillery attack on various targets much more than usual.

Thinker prioritizes naval invasions if the closest enemy is located on another continent. Otherwise, most of the focus is spent on building land and air units. At any given time, only one priority landing zone can be active for the AI. Maximum distance for invasions depends slightly on pathfinding but it should be possible on all Huge maps.

Base garrisoning priorities are also handled entirely by the new logic which tries prioritize vulnerable border bases much more than usual. Air units also utilize the same priorities when deciding where to rebase. You might notice there's less massive AI stacks being rebased around for no meaningful reason. Instead Thinker tries to rebase the aircraft in much smaller stacks to more bases so that they can cover a larger area.


Revised tech costs
==================
In the original game, research costs were mainly decided by how many techs a faction had already researched. That meant the current research target was irrelevant as far as the costs were concerned. For speed runs, it made sense to avoid researching any techs that were not strictly necessary to keep the cost of discovering new techs as low as possible.

The config option `revised_tech_cost` attempts to remake this mechanic so that the research cost for any particular tech is fixed and depends mainly on the level of the tech. This follows the game design choices that were also made in later Civilization games.

For example, in the default tech tree, Social Psych is level 1 and Transcendent Thought is level 16. Enabling this feature should notably delay the tech race in mid to late game. See also [a helpful chart](https://alphacentauri2.info/index.php?action=downloads;sa=view;down=355) of the standard tech tree. The base cost for any particular tech is determined by this formula:

    5 * Level^3 + 75 * Level

Here are the base costs for standard maps:

| Level | Labs  |
|-------|-------|
|     1 |    80 |
|     2 |   190 |
|     3 |   360 |
|     4 |   620 |
|     5 |  1000 |
|     6 |  1530 |
|     7 |  2240 |
|     8 |  3160 |
|     9 |  4320 |
|    10 |  5750 |
|    11 |  7480 |
|    12 |  9540 |
|    13 | 11960 |
|    14 | 14770 |
|    15 | 18000 |
|    16 | 21680 |

The idea here is that level 1-3 costs stay relatively modest and the big cost increases should begin from level 4 onwards.
This feature is also designed to work with `counter_espionage` option. Keeping the infiltration active can provide notable discounts for researching new techs if they can't be acquired by using probe teams.

After calculating the base cost, it is multiplied by all of the following factors.

* For AI factions, `tech_cost_factor` scales the cost for each difficulty level, e.g. `tech_cost_factor=84` equals 84% of human cost.
* Multiply by the square root of the map size divided by the square root of a standard map size (56).
* Multiply by faction specific TECHCOST modifier (higher values means slower progress).
* Divide by Technology Discovery Rate set in alphax.txt (higher values means faster progress).
* If Tech Stagnation is enabled, the cost is doubled unless a different rate is set in the options.
* Count every other faction with commlink to the current faction who has the tech, while infiltrated factions count twice. Discount 5% for every point in this value. Maximum allowed discount from this step is 30%. Only infiltration gained using probe teams counts for the purposes of extra discount, the Empath Guild or Planetary Governor status is ignored in this step.
* If `cheap_early_tech` is enabled, apply a decreasing discount from 60% to 0% until the first 16 techs are discovered.

The final cost calculated by this formula is visible in the F2 status screen after the label "TECH COST". Note that the social engineering RESEARCH rating does not affect this number. Instead it changes "TECH PER TURN" value displayed on the same screen.


Expiring infiltration feature
=============================
Normally establishing infiltration with a probe team on another faction is permanent and cannot be removed in any way. In multiplayer games this can be especially unbalanced. Thinker provides a config option `counter_espionage` to make infiltration expire randomly based on a variety of factors.

When enabled, for each turn for every infiltration active to the current faction (defender), a score is calculated and rolled against a random chance. If the roll succeeds, both factions will get a message of removed infiltration on their turn and the infiltration stops immediately. While allied to another faction or having the Empath Guild, infiltration will never expire. The score is based on these factors:

* Get the current social engineering PROBE rating for both factions.
* Having Hunter Seeker Algorithm or Nethack Terminus will give +1 bonus each.
* If a human player faction is defending, give extra +2 bonus for the defender.
* Calculate DefenderProbeScore - AttackerProbeScore.
* Clamp the value to -7 to +7 range.

The chance to discover infiltration is 0.5% at -7 score, 1.2% at +0 score, and 3.3% at +7 score (maximum possible). Thus having any kind of advantage over the other faction will give a lopsided bonus for the defender by design. In this mechanic, it is important to have a better PROBE rating or at least parity to maintain the infiltration.


SMAC in SMACX mod
=================
Thinker includes the files necessary to play a game similar to the original SMAC while disabling all expansion-related content. See the original release posts of SMAC in SMACX [here](https://github.com/DrazharLn/smac-in-smax/) and [here](https://alphacentauri2.info/index.php?topic=17869.0).

Thinker also adds support for custom factions that are not aliens while using this feature. Installing this mod doesn't require any extra steps since the files are bundled in the zip file.

1. Download the latest version that includes `smac_mod` folder in the zip.
2. Extract all the files to Alpha Centauri game folder.
3. Open `thinker.ini` and change the configuration to `smac_only=1` ***OR*** start the game with command line parameter `terranx_mod.exe -smac`
4. When smac_only is started properly, the main menu will change color to blue as it was in the original SMAC, instead of the SMACX green color scheme.
5. The game reads these files from `smac_mod` folder while smac_only is enabled: alphax.txt, helpx.txt, conceptsx.txt, tutor.txt. Therefore it is possible to keep the mod installed without overwriting any of the files from the base game.
6. To install custom factions while using this mod, just edit `smac_mod/alphax.txt` instead of the normal `alphax.txt` file and add the faction names to `#CUSTOMFACTIONS` section. Note that you only have to extract the faction definitions to the main game folder because the same files are used in smac_only mode.

See below for some custom faction sets. The factions can be played in both game modes and they are also fairly balanced overall.

* [Sigma Faction Set](https://alphacentauri2.info/index.php?action=downloads;sa=view;down=264)
* [Tayta's Dystopian Faction Set](https://alphacentauri2.info/index.php?topic=21515)


Recommended alphax.txt settings
===============================
Proposed alphax.txt settings for Thinker can be found from [this file](docs/alphax.txt). The purpose of these edits is to cause minimal differences to the game defaults, and rather fix bugs and improve the random map generation. That means the tech tree or production cost values are not modified, for example. The changes are summarized below:

1. Fix: Enabled the "Antigrav Struts" special ability for air units as stated in the manual.
2. Fix: Disabled the "Clean Reactor" special ability for Probe Teams because they already don't require any support.
3. The frequency of climate change effects is reduced. Thinker level AIs regularly achieve much higher mineral production and eco damage than the AIs in the base game. As a result, excessive sea level rise may occur early on unless this is modified.
4. WorldBuilder is modified to produce bigger continents, less tiny islands, more rivers and ocean shelf on random maps.
5. Large map size is increased to 50x100. Previous value was nearly the same as a standard map.
6. Descriptions of various configuration values have been updated to reflect their actual meaning.
7. Add new predefined units Trance Scout Patrol and Police Garrison.


Compatibility with other mods
=============================
While it is possible to run both Thinker and PRACX at the same time, this combination of patches will not receive the same testing than the normal configuration, so some issues are likely. Additionally some optional features provided by Thinker will be disabled while running PRACX because they would patch conflicting areas of the game binary. These disabled feature include:

* Smooth scrolling config option.
* Windowed mode config option.
* Any keyboard/mouse shortcuts provided by Thinker.
* ALT+T version menu will not be available in this case.

Scient's patch is already included in the Thinker binary and does not need any additional installation steps from the user. In any case, it is possible to install modified txt files provided by Scient's patch. Yitzi's patch or any other mod that uses a custom patched game binary is NOT supported while running Thinker. Note also that Yitzi's patch uses an incompatible version of alphax.txt that can't be used by the vanilla game binary or Thinker.


Features not supported
======================
Currently the features listed here are not supported while Thinker is enabled. The list may be subject to change in future releases. Some requested features are not feasible to implement due to the limitations of patching a game binary.

1. Network multiplayer (excluding PBEM) is not supported because of the large amounts of extra code required to synchronize the game state across computers.
2. More factions/units/bases. These limits were hardcoded in the game binary at compilation time and are not feasible to change without a full open source port.
3. Some custom scenario rules in "Edit Scenario Rules" menus are not supported fully. This will not affect randomly generated maps. However these rules are supported: `No terraforming`, `No colony pods can be built`, `No secret projects can be built` and `No technological advances`.


Known bugs
==========
1. While `collateral_damage_value` is set to 0, the game might still display temporary messages about collateral damage being inflicted on units on the stack, but none of them will actually take any damage.
2. The faction selection dialog in game setup is limited to showing only the first 24 factions even if installed factions in alphax.txt exceed this number.
3. Maximum supported map size by Thinker is 256x256. This is also the largest map that the game interface will allow selecting.


Other patches included
======================
Some notable game engine patches included with Thinker may not have their separate config option, but they are listed here.

1. Base governors of all factions will now prefer to work borehole tiles instead of always emphasizing food production. The patch makes governors assume borehole tiles produce 1 food but this will not affect the actual nutrient intake or anything else beyond tile selection.
2. Make sure the game engine can read movie settings from "movlistx.txt" while using smac_only mode.
3. Fix issue where attacking other satellites doesn't work in Orbital Attack View when smac_only is activated.
4. Fix engine rendering issue where ocean fungus tiles displayed inconsistent graphics compared to the adjacent land fungus tiles.
5. Fix game showing redundant "rainfall patterns have been altered" messages when these events are caused by other factions.
6. Fix a bug that occurs after the player does an artillery attack on unoccupied tile and then selects another unit to view combat odds and cancels the attack. After this veh_attack_flags will not get properly cleared and the next bombardment on an unoccupied tile always results in the destruction of the bombarding unit.
7. Patch AI vehicle home base reassignment bug. This change inverts the old condition where an apparent oversight made the engine to reassign vehicles to bases with mineral_surplus < 2.
8. Patch the game engine to use significantly less CPU time when idling using a method similar to [smac-cpu-fix](https://github.com/vinceho/smac-cpu-fix/). Normally the game uses 100% of CPU time which can be be a problem on laptop devices.
9. When capturing a base, Recycling Tanks and Recreation Commons are not always destroyed unlike previously. They are sometimes randomly destroyed like other facilities.
10. Fix faction graphics bug that appears when Alpha Centauri.ini has a different set of faction filenames than the loaded scenario file. If the game has an incorrect faction set, the patch will quit to main menu so that the correct faction set will be available when the save is reloaded. This bug only occurs with scenario files, while regular save games are unaffected.
11. Patch the game to use the Command Center maintenance cost that is set in alphax.txt. Normally the game would ignore this value and make the cost dependent on faction's best reactor value which is inconsistent with the other settings for facilities in alphax.txt.
12. Sometimes the base window population row would display superdrones even though they would be suppressed by psych-related effects. The patch removes superdrone icons from the main population row and uses regular worker/drone/talent icons if applicable, so drones should not be displayed there if they are suppressed by psych effects. To check the superdrone status, open the psych sub window.
13. Fix visual bug where population icons in base window would randomly switch their type when clicking on them.
14. Patch AIs to initiate much less diplomacy dialogs when the player captures their bases. Previously this happened at least once for every turn the AI loses any bases and would repeat the same dialog every time if the player didn't agree to the peace terms. The patch makes the initiation of dialog more dependent on random chance unless the AI would finally accept surrender terms.
15. Patch any faction with negative research rating to start accumulating labs on the first turn. In vanilla rules each negative point results in an additional 5 turn delay before the faction starts accumulating labs (e.g. Believers had a 10 turn delay).
16. Patch genetic warfare probe team action to cause much less damage for any units defending the base. In vanilla game mechanics even one attack instantly inflicted almost 80% damage. In the patched version population loss mechanic is unaffected, but even multiple attacks should do substantially less damage for defender units.


Scient's patch
==============
Thinker includes the binary patches from both [Scient's patch v2.0 and v2.1](https://alphacentauri2.info/index.php?action=downloads;sa=view;down=365). The differences between these versions include only changes to the txt files to prevent the game from crashing when opening certain datalinks entries.
Installing the modded txt files is purely optional, and Thinker does not include those files by default. The following fixes from Scient's binary patch are included in `terranx_mod.exe`.

1.  [BUG] If a faction's cumulative PROBE value is greater than 3 (SE morale, covert ops center) it is possible to "mind control" their bases when they should be immune. If the University uses SE Knowledge putting PROBE value down to -4, it would act as if it were 0 erroneously increasing "mind control" costs. After patch, PROBE values greater than 3 will always be immune to regular probes and values less than -2 will be treated as if they were -2.
2.  [CRASH] It is possible usually on larger maps that scrambling air interceptors would cause the game to crash. Even when the game didn't crash incorrect altitude values were being used in checks. Both of these have been fixed.
3.  [CRASH][SMAX] If you open up Design Workshop as Caretakers/Usurpers and switching back and forth between colony pod and other "equipment" you can cause text in "special ability" to become overwritten and crash once you exit Design Workshop. Patch increases memory allocation used to manipulate the caviar animation files (cvr) and may fix other buffer related problems other than the one noted. (credit to WBird784 for original patch)
4.  [EXPLOIT] Using the right click menu to airdrop calls the "move to" code directly and bypasses all the checks such as if unit is in airbase/base or has already moved. Since the airdrop hotkey "i" correctly checks these restrictions, the patch makes the right click menu use the hotkey checks before directly moving the unit.
5.  [BUG/EXPLOIT] Setting more than one patrol waypoint with spacebar causes the values to be stored incorrectly. If only two waypoints are set then it is just a display issue showing an incorrect amount of waypoints when clicking on unit. If three waypoints are set, it causes the unit's morale to be set as one of coordinates usually boosting it to demon boil / elite status. Also, with three waypoints the final waypoint gets set to some far off random location usually (0,0) breaking three point patrol completely.
6.  [BUG][SMAX] Transport unit's special ability "Repair Bay" is rendered useless due to an incorrect check that would only give the healing bonus to ground transports. Now, it will give bonus to all ground units except ground transports.
7.  [BUG] Fixed a check that was ending the turn for certain air units (choppers/missiles/grav) when entering a base which had no adjacent enemy units. There is still a check which will end the unit's turn if when entering a base it has less then one turn remaining unless an enemy is adjacent to base.
8.  [EXPLOIT] Inside base UI, after opening up the production queue window it is possible to then open the hurry command window, switch between bases and complete projects for less then their actual value. Patch fixes a clean up issue with queue panel preventing certain parts of UI from becoming clickable when they shouldn't.
9.  [EXPLOIT] When clicking on an unexplored tile, the map recenters on that square. However, if the tile contains a base the map doesn't recenter. Patch fixes it so when an unexplored tile with a base is clicked, the game will recenter like the rest of the tiles.
10. [CRASH] When moving units near or at the poles, it is possible for y coord to exceed the map bounds and crash the game. Patch adds handling to prevent y coord from going over min/max.
11. [EXPLOIT] It is possible to change another faction's workers via Base Ops (F4) if you have that faction infiltrated. Patch fixes it so clicking citizens of another faction in Base Ops has no effect similar to garrisoned units.
12. [BUG] Using "Go to home base" (shift-g) command sends the unit to closest base rather than it's actual home. Patch retrieves unit's home base and sets a "go to" waypoint similar to how a unit is recalled from within base UI. If a home base cannot be found (independent) the unit will go to the nearest base.
13. [EXPLOIT] It is possible to give airdrop or artillery ability to an unit who didn't have it. This is done by using hotkey (I or F) on an unit that has the ability then switching units in bottom center window. The mouse cursor would still have airdrop/artillery up and it is then possible to use it with the new unit bypassing all the checks if unit can use ability. Patch resets the cursor when clicking unit selection window.
14. [BUG] Fixed an unit check to use the "Max artillery range" value defined in #Rules (alphax.txt) rather than a hardcoded value of 2. This fix is geared toward modders so players who use an unmodified version of alphax.txt will see no change.
15. [BUG][SMAX] Enhanced probes (Algorithmic Enhancement) are now able to mind control bases/units normally immune due to high SE morale as stated in SMAX manual. For units it's purely the morale SE value so >=3 acts as if it were 2 (cost doubled). For bases, the value is calculated from morale SE and any base facility modifiers (Covert Ops: +2, Genejack: -1). If final value is >=3, it acts as if it were 2.
16. [BUG] Squares with both a borehole and nutrient bonus don't receive the nutrient bonus. If the "Borehole Square" nutrient value defined in #RESOURCEINFO (alphax.txt) is set to non-zero, a square with borehole will erroneously act as if it has a nutrient bonus. The patch fixes a check whether or not to give a borehole square a nutrient bonus from checking if borehole nutrient value is non-zero to checking if nutrient bonus is actually present in square.
17. [BUG][SMAX] While loading the ambient sound file for game, there is a mistake in faction id check for Believers causing it to use SMAX default of "aset1.amb". Patch fixes check so Believers will now use their correct ambient sound file "bset1.amb".
18. [BUG][SMAX] Enhanced probes don't receive a penalty to survival probability when target faction has built Hunter-Seeker Algorithm. Instead, the success probability is erroneously given the penalty for a second time after it has already been displayed in UI. This could cause diminished success rate when it should have been higher. Patch corrects check so survival rather than success probability is modified.
19. [UI] Removed a check that would while displaying probe success/survival probabilities drop one if they're both the same. For example, (50%, 50%) would just be (50%).
20. [MISC] Disabled cpu check which serves no purpose other than preventing the game from starting on newer cpu's. Patch enables ForceOldVoxelAlgorithm=1 in code by default and suppresses annoying warning.
21. [BUG][SMAX] Sealurk units now do not get a penalty when moving onto Sea Fungus like Isle of Deep.
22. ***Removed***
23. [MISC] Fixed a registry check at startup (Vista/Win7) whether a complete installation has been performed. Prevents #FILEFIND_NOCD from being incorrectly displayed.
24. [BUG] Units without the Amphibious Pods ability can no longer move to a land tile from a sea base without there being a transport in land tile.
25. [BUG][SMAX] Prevent the Caretakers from being given the possibility of building secret project "Ascent to Transcendence" which goes against their philosphy and would cause them to declare war on themselves if built.  Problem with logic, code exists which should of prevented it from displaying but didn't.
26. [BUG] Allow tiles with more than one probe ability to evict probes (more than one unit, probe has to be top most) ; when evicting probe, only send top unit to home base -> prevents non-probe units from being booted as well. -> using same code used to return probes to base after successful mission
27. [BUG][SMAX] Fixed bug when AI successfully completes probe action of freeing a captured faction leader (FREEWILLY/FREEWHO) where it would reset a non-captured faction.  The problem was that AI would always try to free the very first faction (usually PC) regardless of whether they were elminated or not.  Fix now obtains all potential faction leaders to free and randomly picks one.
28. [BUG][SMAX] Add references to use a new file (movlistx.txt) for SMAX specific info after secret project movies rather than one shared between SMAC/SMAX (movlist.txt).
29. [MOD] Add "Nessus Canyon" landmark placement on random map generation.
30. [BUG] Fixed "attacking along road" combat bonus in alpha/x.txt.  Set to 0 by default.
31. [BUG/MOD] Fixed NEWRESOURCE and PETERSOUT events to display the correct corrosponding image to match resource type.
32. [MOD] Add ability to set individual datalink entries for armor and reactors (ARMORDESC#/REACTORDESC#) in help/x.txt.
33. [MOD] Enable the display of Sea Formers unit (UNITDESC4) in datalinks.  Also, hide '*' from showing in name.
34. [BUG] Fixed logic in PB code to prevent ODP/Flechette defenses being called a 2nd time (with a chance for defenses to fail) even after PB had already been shot down.  Each faction has one chance to defend against incoming missile based on closest base or unit to PB blast.  However, there is secondary check to give the tile owner a chance to defend against PB even if they have no units or bases in this space.  This check didn't take into account whether or not PB had already been shot down.
35. [BUG/MOD][SMAX] Fungal Missiles would spawn MW in ocean tiles -> fix by spawning IoD instead and adding in one Sealurk instead of Fungal Tower.
36. [BUG] Fixed a bug that would display NERVEGAS message even when attack on base failed -> checking attacker health
37. [MOD] Add ability to set reactor (1-4) for #UNITS in alpha/x.txt (ex. "Colony Pod,..., 00000000000000000000000000,4" -> gives Colony Pods Singularity Engine).  To do so, just add comma after Abil with value of reactor you want for unit.  If no value is set, it defaults to "1" like original code.  For SMAX only, there are two exceptions for Battle Ogre MK2 and Battle Ogre MK3 where default isn't "1" but "2" and "3" respectively.  The Ogre default can still be overriden if you set a different reactor value in alpha/x.txt.
38. [BUG] Allowing colony pods to be added to bases with fungus on tile ; allowing sea/land pods to be added to existing sea/land bases ignoring building restriction
39. [BUG] #DISEMBARKFAIL wasn't being displayed due to incorrect logic check ; when you try to move a transport with no units on board (L) onto land this message should display (doesn't include moving into land base, does include two transports on same tile one without units and other with) -> update: added additonal check so #DISEMBARK code wasn't being particially loaded for non-transport units
40. [BUG] Changing start date for Perihelion event to be 2160 from 2190.  This is to be consistent with info about Planet and cycle from readme regarding 80 year cycles (20 years near, 60 years far).
41. [BUG][SMAX] FM would sometimes display #FUNGMOTIZED due to faction id of FM memory location would change and instead would use faction id of MW.  Fixed by storing faction id in variable.
42. [CRASH] Under rare circumstances, the game would crash when an AI faction with no bases attempted to upgrade a unit.
43. [BUG] Abandoning a base after building a colony pod no longer skips the base production of the next base.  This was caused by the upkeep function using incorrect base values after the abandoned base was destroyed.  Fixed so acts like #STARVE.
44. [BUG] Movement to and from pact bases should be identical to your own bases. Since a non-amphibious unit can move from a transport into your own sea base, it should be able to move from a transport into a pact mate's base.
45. [BUG][SMAX] Interludes 6 and 7 would display incorrect string variables specific to Caretakers or Usurpers for non-Alien factions.  They now show correct values for human factions as well as Aliens.
46. [BUG] Interlude 6 would sometimes be triggered by native life forms causing issues with variables and not making sense, this interlude (and it's follow up #7) are designed for another specific faction.  Add check to skip if faction killing unit is alien.
47. [BUG] Fixed display when scenario editor is enabled for changing former owner inside a base (#CHANGEFORMER) since menu didn't load correctly.
48. [BUG] Fixed the parsing of the "Retool strictness" value in alpha/x.txt so "Never Free" works correctly. This would only apply if you wanted to give a retooling penalty when switching to "Secret Projects".
49. [BUG][MOD] Factions with the FREEPROTO flag (Spartans) will gain free retooling in their bases as long as the production switch is within the same category (unit to unit, base facility to base facility) and they've discovered the necessary tech ("Advanced Subatomic Theory"). This is to resolve the issue with FREEPROTO factions never being able to gain the undocumented retooling ability of "Skunkworks" when it is fairly clear that they should.
50. [EXPLOIT] Using the right click "Save current list to template" and "Load template into list" features of base queue can be used to bypass retooling completely. Fixed so these queue template features only save and load the actual queue and not affect the item currently in production.
51. [BUG] When drilling an aquifer, there isn't a check whether a river already exists in the initial square. Now, it checks the the initial square as well as the eight square around it.
52. [BUG] Fixed an issue where diplomacy dialog could be incorrectly displayed due to faction id value being set incorrectly. This is best exhibited where Progenitors switch into "Human" dialog syntax.
