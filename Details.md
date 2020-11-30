
Main features
=============
Thinker has several advanced planning routines that enhance the base game AI to manage the complexities of colony building in Alpha Centauri. In the original game, many of the AI aspects were left underdeveloped while the computer factions struggled to deal with various punitive penalties. This left the single player experience severely lacking, since the AI would have no counter to many simple strategies.

Thinker does not have any special save game format, so it's possible to open an old save and have the factions switch to the new AI and vice-versa. None of the config options are preserved in the save game either, but the units or resources spawned at the game start will remain.

Enabling Thinker AI will affect many AI behaviors as it is described in the list below. Thinker will not change the automated behaviour of units or governors in player-controlled factions.

For testing purposes it is also possible to run old/new AIs side-by-side by setting `factions_enabled=3` or similar. In that case, only the factions in the first 3 slots will use Thinker AI if they are not player-controlled. By default, `factions_enabled=7` setting enables Thinker AI for all computer factions.

1. Thinker fully controls the movement of colony pods, terraformers and crawlers to manage the base placement and production much more effectively than usual.
2. New combat routines for land-based units will attempt to counter-attack nearby enemy units more often. If the odds are good enough, hasty attacks are executed more often than usual. The AI will fight the native units more aggressively, and it will also try to heal its units at monoliths.
3. Thinker base production AI will decide every item that is produced in a base. The build order can differ substantially from anything the normal AIs might decide to produce, and the difference can be easily noted in the vastly increased quantity of formers and crawlers the AIs might have.
4. Social AI feature will decide the social engineering models the AI factions will choose. It will attempt to take into account the various cumulative/non-linear effects of the society models and any bonuses provided by the secret projects. The AI is now capable of pop-booming if enough growth is attainable, and it will also try to avoid pacifist drones by switching out of SE models with too many police penalties. All the SE model effects are moddable because the AI is not hardcoded in any particular choices. This feature is also capable of managing all the custom factions.
5. Tech balance will assign extra importance on some specific techs when selecting what to research: requirements for formers, crawlers, recycling tanks, children's creches, and the 3 technologies to allow the production of more than 2 resources per square. If these items don't have any requirements, or the techs are not available to research, the tech progression is unaffacted by this feature. It will also not affect player faction research in any way.
6. Hurry items feature is able to use AI energy reserves to occasionally hurry base production. Items that can be hurried include most basic facilities, formers, and (only rarely) combat units. The AI will never rush secret projects in this way, but sometimes they can be completed quickly with overflow minerals.

In addition to the bugfix patches listed separately, items that are listed after "Game Engine Patches" in `thinker.ini` apply to all factions regardless of `factions_enabled` setting. Some of these patches include:

1. Base governors of all factions will now prefer to work borehole tiles instead of always emphasizing food production. The patch makes governors assume borehole tiles produce 1 food but this will not affect the actual nutrient intake or anything else beyond tile selection.
2. All landmarks that are placed on random maps can now be configured from `thinker.ini`. Nessus Canyon is also available but disabled by default.
3. New `faction_placement` algorithm tries to balance faction starting locations more evenly across the whole map area while avoiding unusable spawns on tiny islands. The selection also takes into account land quality near the spawn. The effect is most noticeable on huge map sizes. It is also possible to add extra nutrient bonuses for each land-based faction by using the `nutrient_bonus` setting.
4. New `design_units` feature will introduce custom probe teams, armored crawlers, gravship formers, and AAA garrison prototypes for the computer factions.
5. All expansion related content can be disabled by using the `smac_only` setting, see further below.
6. AI mineral/nutrient production cost factors for each difficulty level can be changed from the `cost_factor` setting. Does not affect other difficulty related modifiers.
7. Content (non-drone) base population for each difficulty level can be adjusted from `content_pop_player` and `content_pop_computer` variables. By default these have the same values than vanilla game mechanics.
8. Game draws a white marker around the population label of HQ bases to identify them more easily. Optional `auto_relocate_hq` feature will import a game mechanic from Civilization 3 where lost/captured headquarters are automatically moved to another suitable base.


Revised tech costs
==================
In the original game, research costs were mainly decided by how many techs a faction had already researched. That meant the current research target was irrelevant as far as the costs were concerned. For speed runs, it made sense to avoid researching any techs that were not strictly necessary to keep the cost of discovering new techs as low as possible.

The config option `revised_tech_cost` attempts to remake this mechanic so that the research cost for any particular tech is fixed and depends mainly on the level of the tech. This follows the game design choices that were also made in later Civilization games. For example, in the default tech tree, Social Psych is level 1 and Transcendent Thought is level 16. Enabling this feature should notably delay the tech race in mid to late game. See also [a helpful chart](https://alphacentauri2.info/index.php?action=downloads;sa=view;down=355) of the standard tech tree. The base cost for any particular tech is determined by this formula:

    6 * Level^3 + 74 * Level - 20

For example, on standard maps level 1 techs cost 60 labs before other factors are considered. The idea here is that level 1-3 costs stay relatively modest and the big cost increases should begin from level 4 onwards. After calculating the base cost, it is multiplied by all of the following factors.

* For AI factions, each unit of `cost_factor` applies a 8% bonus/penalty, e.g. `cost_factor=10` equals 100% of human cost while `cost_factor=7` equals 76% of human cost.
* Multiply by the square root of the map size divided by the square root of a standard map size (56).
* Multiply by faction specific TECHCOST modifier.
* Multiply by Technology discovery rate set in alphax.txt.
* If tech stagnation is enabled, multiply by 1.5.
* If 2 or more factions with commlink to the current faction has the tech, multiply by 0.75.
* If only 1 commlink faction has it, multiply by 0.85.

The final cost calculated by this formula is visible in the F2 status screen after the label "TECH COST". Note that the social engineering RESEARCH rating does not affect this number. Instead it changes "TECH PER TURN" value displayed on the same screen.


SMAC in SMACX mod
=================
Thinker includes the files necessary to play a game similar to the original SMAC while disabling all expansion-related content. See the original release posts of SMAC in SMACX [here](https://github.com/DrazharLn/smac-in-smax/) and [here](https://alphacentauri2.info/index.php?topic=17869.0).

Thinker also adds support for custom factions that are not aliens while using this feature. Installing this mod doesn't require any extra steps since the files are bundled in the zip file.

1. Download the latest version that includes `ac_mod` folder in the zip.
2. Extract all the files to Alpha Centauri game folder.
3. Open `thinker.ini` and change the configuration to `smac_only=1` ***OR*** start the game with command line parameter `terranx_mod.exe -smac`
4. When smac_only is started properly, the main menu will change color to blue as it was in the original SMAC, instead of the SMACX green color scheme.
5. The game reads these files from `ac_mod` folder while smac_only is enabled: alphax.txt, helpx.txt, conceptsx.txt, tutor.txt. Therefore it is possible to keep the mod installed without overwriting any of the files from the base game.
6. To install custom factions while using this mod, just edit `ac_mod/alphax.txt` instead of the normal `alphax.txt` file and add the faction names to `#CUSTOMFACTIONS` section. Note that you only have to extract the faction definitions to the main game folder because the same files are used in smac_only mode.

See below for some custom faction sets. The factions can be played in both game modes and they are also fairly balanced overall.

* [Sigma Faction Set](https://alphacentauri2.info/index.php?action=downloads;sa=view;down=264)
* [Tayta's Dystopian Faction Set](https://alphacentauri2.info/index.php?topic=21515)


Recommended alphax.txt settings
===============================
Proposed alphax.txt settings for Thinker can be found from [this file](docs/alphax.txt). The purpose of these edits is to cause minimal differences to the game defaults, and rather fix bugs and improve the random map generation. That means the tech tree or production cost values are not modified, for example. The changes are summarized below:

1. The frequency of global warming effects is reduced. Thinker level AIs regularly achieve much higher mineral production and eco damage than the AIs in the base game. As a result, excessive sea level rise may occur early on unless this is modified.
2. Large map size is increased to 50x100. Previous value was nearly the same as a standard map.
3. WorldBuilder is modified to produce bigger continents, less tiny islands, more rivers and ocean shelf on random maps.
4. Descriptions of various configuration values have been updated to reflect their actual meaning.
5. [BUG] Enabled the "Antigrav Struts" special ability for air units as stated in the manual.
6. [BUG] Disabled the "Clean Reactor" special ability for Probe Teams because they already don't require any support.
7. Add new predefined units Trance Scout Patrol and Police Garrison.


Features not supported
======================
Currently the features listed here are not supported while Thinker is enabled. The list may be subject to change in future releases. Some requested features are not feasible to implement due to the limitations of patching a game binary.

1. Network multiplayer (excluding PBEM) is not supported because of the large amounts of extra code required to synchronize the game state across computers.
2. More factions/units/bases. These limits were hardcoded in the game binary at compilation time and are not feasible to change without a full open source port.
3. Some custom scenario rules in "Edit Scenario Rules" menus are not supported fully. This will not affect randomly generated maps. However these rules are supported: No terraforming, No colony pods can be built and No secret projects can be built.


Known bugs
==========
1. While `collateral_damage_value` is set to 0, the game might still display temporary messages about collateral damage being inflicted on units on the stack, but none of them will actually take any damage.
2. The faction selection dialog in game setup is limited to showing only the first 24 factions even if installed factions in alphax.txt exceed this number.


Bugfix patches included
=======================
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
