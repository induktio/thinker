
# Thinker mod release changelog

## Version 4.4 (2024-07-26)
* Add option long_range_artillery to enable increased range for ship-based artillery when the special ability is used. Land-based artillery remains the same as before.
* Cost factor for Artillery ability is changed to "increases with speed value" instead of "armor+speed value".
* Minor changes on AI ship designs and prototype naming rules to be more consistent. Longer range artillery ships use only Battleship names.
* Add new predefined unit Drop Colony Pod. AI is also able to use such units when available.
* Add option intercept_defense_bonus for "Air superiority unit vs. air unit" in alphax.txt to also affect scrambled interceptors.
* Add option label_unit_reactor to customize how reactors are displayed in the prototype names.
* Thinker governors are able to build secret projects and hurry production also in player bases. Hurry actions will display a notification in the message log.
* Thinker is also able to automate player-owned crawlers. Most often they will attempt to convoy nutrients and minerals.
* Interlude and credits screen planet backgrounds are scaled to fit all resolutions.
* Fossil Field Ridge is placed on ocean shelf tiles when modified_landmarks is enabled.
* Clinical Immortality provides one extra talent per base instead of two as mentioned in the manual.
* Base production picker will not show Paradise Garden as buildable if the base already has Punishment Sphere.
* Minor clarifications on smac-in-smacx mod documentation to remove mentions of resolved issues.
* Fix multiple issues with the user interface by modifying script entries (see script_label option and Details.md).
* Fix issue where the game might crash when first opening an old savegame and clicking some crawler unit on the world map.


## Version 4.3 (2024-05-26)
* Random map generator allows placing multiple similar landmarks when the related option value is greater than one.
* Modify Mind Control probe action to only subvert units inside the base and not on adjacent tiles to balance for the relatively cheap cost for this action.
* Modify Total Thought Control probe action so that the AI will not always sneak attack. This action can be still used to subvert bases without declaring war but it applies a notable diplomatic penalty between the factions.
* Many changes on AI diplomacy code related to tech trading, energy loans, base swaps and threats to cancel the current alliance.
* Add options modify_upgrade_cost, normal_elite_moves and native_elite_moves.
* Add possibility to scroll the prototype list in design workshop with the mouse wheel.
* Change energy market crash event icon to genwarning_sm.pcx while previously it was markbm_sm.pcx.
* Fix reactivating skipped units not working for aircraft units that have not moved.
* Fix magtube_movement_rate issue with aircraft units when the cost is non-zero.
* Fix chopper movement points being modified when the combat is canceled from the confirmation dialog.
* Fix some game preferences being reset when starting a new game with ignore_reactor_power enabled.
* Fix Mind Control issue when capturing bases that have non-allied units owned by a third faction inside them.
* Fix datalinks window not showing the first character for Sea Formers units.
* Fix issue with event_market_crash option that occurred during turn processing.


## Version 4.2 (2024-04-12)
* Game setup screen will now adjust the planet preview background based on selected properties on all resolutions.
* Base window support tab shows a terrain view that is zoomable with the mousewheel or Ctrl+Up/Down.
* Players on two lowest difficulty levels will start accumulating labs immediately instead of 5 turns later (early_research_start).
* Add event_market_crash option to select if the event is enabled. This can now reduce energy reserves by 1/2 when the faction has more than 1000 credits.
* Add biology_lab_bonus option to adjust the research bonus granted by the facility. AI may sometimes build this facility if the benefit is large enough.
* AI will also consider suitable non-combat predefined units for production even if the tech for corresponding non-combat module is not available.
* Planetpearls option is set by default to slightly reduce planetpearls income.
* Patch Stockpile Energy when it enabled double production if the base produces one item and then switches to Stockpile Energy on the same turn gaining additional credits.
* Stockpile Energy also shows extra credits gained per turn on the base resource view.
* Minor changes on AI priorities with naval pathfinding and objective bases in scenario mode.
* Fix base governor not building prototype units in some cases if the prototype cost is zero.
* Fix startup issues with the game launcher when the keyboard shortcuts were not working.
* Fix issue when faction credits are less than the minimal hurry cost, the default payment will be equal to credits available.
* Fix production picker showing incorrect Stockpile Energy output in the selection list when Planetary Energy Grid is built.
* Fix world map shifting the bottom row of tiles to the wrong side of screen when the map is zoomed out.
* Fix rare issue where the game freezes during turn processing if some of the combat units are automated.


## Version 4.1 (2024-03-28)
* Replace older DirectDraw and windowed mode options with video_mode option which also allows to select fullscreen mode on custom resolution.
* It is now possible to toggle between windowed and custom resolution modes by pressing Alt+Enter.
* Add command line options -native and -screen to set specific resolution modes.
* Add options dialog item to select if the game window is always minimised when not activated.
* Add options to change ambient music tracks for any faction in thinker.ini. Expansion factions now use more varied music selections by default.
* Add much more base names for most factions.
* Planet Cult defaults to early start as with other non-alien factions.
* Minor changes to Monsoon Jungle and Borehole Cluster placement when modified_landmarks is enabled.
* Map mirroring config options are renamed in thinker.ini.
* Fix minor issues with mouse wheel zooming on the world map.
* Fix issues where the game would crash on resolutions not divisible by 8. If an unsupported resolution is used, the game will attempt to automatically switch to the closest supported size.


## Version 4.0 (2024-02-16)
* Base window notifications layout is rearranged to show various population counts, nerve staple turns, drone riots, golden age and population boom warnings on the bottom right corner. The notifications are color coded for easier reading.
* It is now possible to adjust special scenario rules for new maps started from the main menu by using ALT+T options dialog.
* Thinker controlled formers now also support other automated subtasks and preferences selections in addition to full automation.
* Add option native_lifecycle_levels to change the lifecycle level for planet-owned units. By default this will increase every 40 turns making it faster than in vanilla game.
* Add option planet_defense_bonus to adjust "Psi attack bonus/penalty per +PLANET" in alphax.txt to also affect defensive combat by half as much.
* Add option time_warp_start_turn for the modified time warp feature.
* Add translation labels for new user interface features in thinker.ini.
* Modify sunspots event to always last for 10 turns as specified by event_sunspots option.
* Prevent the AI from making unnecessary trades where it sells their techs for maps owned by the player. The patch removes two dialogue paths making the AI usually demand a credit payment for any techs.
* AI will rename captured bases only after they are fully assimilated.
* Fix some issues where AI declared war based on calculations using incorrect variables. AI is also less likely to declare war at early stages.
* Fix possible issues with inconsistent captured base facilities when some of the factions have free facilities defined for them.


## Version 3.9 (2023-12-22)
* Add options nerve_staple, event_sunspots, event_perihelion to alter more game rules.
* Nerve staple dialog is now hidden if it is always disallowed by config. When Police rating is insufficient, this will display a separate dialog as usual.
* All the parameters related to starting unit and satellite spawning are renamed. Updated definitions are listed in thinker.ini.
* The game now by default spawns each faction with 2 colony pods on all difficulty levels. Alien factions receive one extra colony pod.
* Fix rare issue where vanilla movement AI sometimes crashed the game during turn processing.
* Fix another issue where the game might create corrupted unit stacks possibly causing the game to freeze.
* Fix visual issues where the game sometimes did not update the map properly when recentering it offscreen on native resolution mode.


## Version 3.8 (2023-10-06)
* Option rare_supply_pods=2 now removes all supply pods from the start when No Unity Scattering is selected.
* Option nutrient_bonus is now applied by default on aquatic and player controlled factions.
* Foreign treaty popup option can also be toggled from the mod config menu.
* Modified time warp creates additional forests or kelp farms near the start location and sometimes removes fungus.
* Number of colony pods on time warp is now equalized across all factions even if they are alien or aquatic.
* Fix issue where AI sometimes lost extra credits by doing unnecessary upgrades during accelerated start.
* Fix issue with chopper_attack_rate calculation with higher values.


## Version 3.7 (2023-09-30)
* Option chopper_attack_rate adjusts how many movement points choppers will expend upon every attack, default is two points.
* Option nerve_staple_mod makes nerve staple effects expire twice as fast when SE Police rating is below a certain value, default is Police < 0.
* Add options time_warp_mod, time_warp_techs, time_warp_projects to choose modded Accelerated Start features for the game.
* Cheap early tech discount is changed to apply for the first 10 techs discovered.
* Small adjustments to name generation rules in new_unit_names option.
* Fix issue with TECHSHARE faction ability ignoring infiltration status when used with smac_only mode.
* Fix issue where Accelerated Start option may sometimes freeze the game when selecting a random secret project for Hive.


## Version 3.6 (2023-03-24)
* Game will now create more accurate prototype names for all unit types (new_unit_names).
* Combat morale effects are updated to reflect manual more closely, most notably with Children's Creche and Brood Pit (modify_unit_morale).
* Fix missing defender bonus mentioned in the manual "Units in a headquarters base automatically gain +1 Morale when defending".
* Fix some issues where the AI didn't determine correctly if it had enhanced probe teams available.
* Base build priorities are reworked to start Aerospace Complex and satellite production sooner.
* Secret projects hurried by the AI will display notifications even during sunspots if the commlink is active.
* If magtube movement rate is set very high (non-zero), the game will limit maximum unit speeds selectively to avoid issues.
* Thinker AI now handles also conventional missiles.
* AI sometimes builds needlejets with dissociative wave special ability when AAA ability is also available.
* AI is capable of longer range orbital insertions whenever the tech or Space Elevator is available.
* AI tries to make sure each base has a suitable armored garrison unit present.
* AI builds Tachyon Fields in border bases more often.


## Version 3.5 (2023-03-04)
* Nerve staple turns and upcoming drone riots warning are shown on the bottom right side corner on base screen.
* If any satellites are active, bonuses from satellites are shown on the left corner on base screen.
* Remaining turns for the captured base extra drones is visible on the psych sub window.
* Patch captured base extra drone effect to last a variable time from 20 to 50 turns depending on the captured base size.
* When a base is captured that was previously owned by active third faction and the time to assimilate the base was more than 10 turns, the previous owner is preserved after capture.
* Whenever additional units are added in the editor mode, these are set as independent units requiring no support by default (editor_free_units).
* Add shortcut ALT+O for scenario editor mode to run the random map generator with a predefined value.
* Add config option to select whether polar caps should be placed on randomly generated maps.
* Rename world_map_labels config option to render_base_info.
* Minor updates to base name lists and map generator settings.
* Draw Hive faction labels with a more visible high contrast text color.
* Fix multiple issues with mouse wheel zooming on the world map and some additional popup windows.
* Fix diplomacy dialog issues when both human and alien factions are involved in a base capture by removing the event that spawns additional colony pods.
* Fix visual bug on some Windows versions that caused random characters to appear on the diplomacy scan display on the world map.


## Version 3.4 (2023-02-18)
* Include hundreds of additional base names for all standard factions. These names are stored in basenames folder.
* Whenever standard base names for any faction run out, the mod creates more variations from basenames/generic.txt.
* Disable drone revolt event from player factions since this event did not happen on AI factions (skip_drone_revolts).
* When moving units into land fungus squares, the movement cost is always limited to full speed of the unit to avoid random waiting (fast_fungus_movement).
* Nerve staple turns are now displayed on a new location on base resource window.
* Fix infiltrate option sometimes disappearing from the probe menu when infiltration is not active.
* Fix issue where world map labels updated one turn too early to indicate an impending drone riot.


## Version 3.3 (2022-12-15)
* Game will now allow reselecting units that have already skipped their turns if they have full movement points available.
* Bases that have sufficient drone control facilities before the growth phase can grow without triggering possible drone riots on the same turn.
* Thinker's map generator now reads all settings from thinker.ini. WorldBuilder in alphax.txt is not used for this purpose.
* New map generator options: world_hills_mod, world_ocean_mod, world_islands_mod.
* Modify revised_tech_cost to take more into account the number of known techs.
* Mod launcher displays more detailed error messages whenever it encounters an issue at startup.
* Fix foreign_treaty_popup displaying same treaty changes multiple times per turn.
* Fix several issues with AI capturing alien artifacts.


## Version 3.2 (2022-12-02)
* New config option: magtube_movement_rate sets the movement speed on magtubes (not enabled by default).
* Expiring infiltration feature has been rewritten to display the expected active turns immediately when infiltration is established.
* Infiltration can now be refreshed once per turn to reset the expiration counter. Probe team may sometimes be lost by this action.
* The amount of techs that grant automatic morale boosts for probe teams is reduced from five to two. These techs are now Pre-Sentient Algorithms and Digital Sentience.
* AI may sometimes build Covert Ops Centers in the late game.
* Many updates and clarifications to smac-in-smacx mod help files.
* Thinker now reads and saves all settings changed from the GUI in thinker.ini. Mod settings are not saved anymore in Alpha Centauri.ini.
* Fix some game startup issues in the launcher that occurred with Windows 10.


## Version 3.1 (2022-02-11)
* New config options: alien_early_start, cult_early_start, perimeter_defense_bonus, tachyon_field_bonus.
* Add option for X/Y map mirroring. This must be enabled separately from ALT+T menu. All landmarks will be disabled if the generated map is mirrored.
* Include cost rebalance increases for Crawler, Needlejet, Copter and Gravship chassis units.
* Copter range is reduced to 8 and Missile range increased to 18. These ranges are for fission reactors.
* AI tech trading has been updated to ask for higher prices whenever the player offers to buy techs for credits.
* AI will not attempt to buy techs when they have less than 100 credits to avoid an issue where they could end up with negative credits.
* Remove Fossil Field Ridge from smac-in-smacx maps since this is expansion-only content.
* Patch Energy Market Crash event to reduce credit reserves only by 1/4 instead of 3/4.
* Fix probe teams and crawlers sometimes being given incorrect move orders.
* Fix issue that causes the AI to recalculate social engineering choices each time 'E' screen in scenario editor mode is opened.
* Fix bug that prevents the turn from advancing after force-ending the turn while any player-owned needlejet in flight has moves left.


## Version 3.0 (2022-01-02)
* Energy loan and base swapping negotiations in diplomacy have been overhauled to make better decisions for the AI.
* AI has new dialogue when they sometimes reject a proposed offer or when they are unable to grant new loans.
* AI tries to evict neutral probe teams from its territory. This will be displayed in a separate popup if foreign_treaty_popup is enabled.
* AI probes will sometimes attempt to subvert adjacent enemy units (only one per tile) with their energy reserves.
* AI is now able to hurry secret projects with energy reserves if it is considered an important project by the AI priorities.
* Secret project hurrying can also be adjusted with base_hurry config option.
* Removed config options: hurry_items, limit_project_start.
* Fix issue that caused sea-based probe teams to be returned to landlocked bases. Probes are now returned to the closest base by the pathfinding distance.
* Fix issue with randomized faction agendas where they might be given agendas that are their opposition social models.
* Additionally randomized leader personalities option now always selects 1 or 2 AI priorities.
* Patch crawler upgrade cost so that it depends only on the mineral row cost difference between the prototypes multiplied by 40. Nano Factory does not affect crawler upgrades anymore.
* Disable legacy upkeep code in the game engine that might cause AI formers to be reassigned to nearby bases that are owned by other factions.


## Version 2.9 (2021-12-17)
* Thinker now uses a new launcher program called thinker.exe to start the mod. It requires having the original terranx.exe version in the game folder.
* Base hurry dialog is overhauled to display the minimal required hurry cost to complete the current production on the next turn.
* Minimal hurry cost is now the default choice when a partial cost is entered instead of the full cost like previously.
* Player bases, formers and colony pods can now be automated by Thinker from ALT+T options menu.
* AI former priorities have been updated to do more cost-benefit calculations between various improvement types such as forest, fungus or boreholes.
* New config options: natives_weak_until_turn, neural_amplifier_bonus, dream_twister_bonus, fungal_tower_bonus.
* AI builds Crawlers, Hologram Threatres and Tree Farms more often.
* AI sometimes nerve staples bases when sunspots are disrupting communications.
* AI tries to fit garrison unit prototypes special abilities in this order: AAA, Comm Jammer, Double Police, Trance, High Morale.
* Update design_units feature to check all possible special abilities rules from alphax.txt for particular weapon/chassis combinations.
* Update new_base_names to also use land names list for sea bases if the other one runs out. Long base names are automatically truncated to 24 characters.
* Default AI cost factor is changed to 12,11,10,9,8,7. The top two levels are the same than previously.
* Revised tech cost also counts allied factions twice when determining the discount for techs known to other factions.
* Fix potential crash when a game is loaded after using Edit Map > Generate/Remove Fungus > No Fungus.
* Fix foreign base names being visible in unexplored tiles when issuing move to or patrol orders to the tiles.
* Fix diplomacy dialog to show the missing response messages (GAVEENERGY) when gifting energy credits to another faction.


## Version 2.8 (2021-11-24)
* Add new rewritten random map generator to the game. It also adds options to set the ocean coverage levels and continent types in a more consistent way.
* Map generator options: new_world_builder, world_sea_levels, world_continents, modified_landmarks.
* Other added options: warn_on_former_replace, simple_hurry_cost, simple_cost_factor, tech_stagnate_rate.
* Renamed options: planetpearls, aquatic_bonus_minerals, alien_guaranteed_techs.
* Faction placement is modified to make spawns on river tiles much more likely.
* It is possible to change more options from Thinker's ALT+T menu. In this case the changes are saved to Alpha Centauri.ini.
* When building or capturing a new base, the mod will automatically copy the saved build queue from Template 1 to the new base.
* Change cheap_early_tech to be enabled by default and the effect is now extended to the first 16 techs.
* AI formers will build slightly less condensers and more forests.
* AI tries to hurry combat unit production more often than previously.
* Modify design_units feature to ensure the AIs will have a ship prototype with the highest rated weapon available.
* Fix faction graphics bug that appears when Alpha Centauri.ini has a different set of faction filenames than the loaded scenario file.
* Patch the game to use the Command Center maintenance cost that is set in alphax.txt.
* Patch the base window population row to remove superdrone icons if they would be suppressed by psych-related effects.
* Fix visual bug where population icons in base window would randomly switch their type when clicking on them.
* Patch AIs to initiate much less diplomacy dialogs when the player captures their bases if they would repeat the same dialog every time.
* Patch genetic warfare probe team action to cause much less damage for any units defending the base.
* Patch terrain drawing engine to render more detailed tiles when zooming out from the default level. Previously the tiles were replaced with blocky, less detailed versions on almost every zoom out level.


## Version 2.7 (2021-10-23)
* Add PSI gate support for AI. They are sometimes built in late stages of the game.
* Add AI ability to steal unescorted artifacts from neutral factions.
* Add AI support for special scenario rule "No technological advances".
* Add new larger than Huge map size 80x160 to alphax.txt.
* New option counter_espionage: provides a mechanic for randomly expiring infiltration on other factions.
* New config options: cheap_early_tech, render_probe_labels, crawler_priority, foreign_treaty_popup.
* When capturing a base, Recycling Tanks and Recreation Commons are not always destroyed unlike previously.
* Many general improvements to AI combat movement code.
* Update base garrisoning priorities to work better in many situations.
* Improve crawler tile selection to better take into account the actual resource yield.
* Rename ac_mod folder to smac_mod for SMAC in SMACX mod.
* Fix rare issue where AI units sometimes freeze trying to attack other units.


## Version 2.6 (2021-05-01)
* New option: skip_faction will exclude some factions from random selection in the game setup.
* New option: conquer_priority provides ability to tweak the AI affinity to build more military units.
* Bases that are about to drone riot on the next turn are now highlighted with a red label on the world map.
* Game now preserves autosave files for each turn in the saves folder. They are overwritten only when the turn is played again.
* Release now contains German translation for alphax.txt provided by Pianoslum.
* Add more checks for AI base production to avoid building too expensive facilities.
* AI can now build additional mineral multiplier facilities in one base. However this can only happen if it doesn't push the base over eco damage threshold.
* AI sometimes builds Punishment Spheres in captured bases.
* AI formers will occasionally drill new rivers.
* AI tries to determine better if it needs land or sea former units.
* AI sometimes does naval landings on the same continent when the distance is shorter than land route.
* More additional tweaks to AI unit movement and scouting priorities.
* Fix game showing redundant "rainfall patterns have been altered" messages when these events are caused by other factions.
* Fix AI not attacking certain aircraft in flight even when it had the chance to do so.
* Fix AI building too many conventional missiles or obsolete planet busters under certain conditions.
* Fix AI not starting to build Ascent to Transcendence when Voice of Planet is operational.


## Version 2.5 (2021-04-11)
* Add new in game menu support for Thinker features (ALT+T shortcut). While a game is loaded, the menu will display elapsed time during that savegame.
* Add menu option to toggle world_map_labels while in game.
* Bases that have Flechette Defense System or Geo Survey Pods are now highlighted with a blue label on the world map.
* Many adjustments to combat movement logic. AI tries to distribute attackers to many more tiles to avoid collateral damage.
* Improve AI pathfinding to take into account 1-tile chokepoints that are occupied by a base.
* AI costs in revised_tech_cost can now be set separately with tech_cost_factor option.
* Rename expansion_factor option to expansion_limit. It is now a fixed limit for all map sizes.
* Replace ignore_reactor_power patch with Will To Power version. This implementation does not require any changes to script.txt and will display combat odds in the vanilla format.
* AI base spacing can now be adjusted with base_spacing and base_nearby_limit options. By default they are the same values than previous versions.
* Add new options: expansion_autoscale, social_ai_bias, new_base_names, disable_alien_guaranteed_techs and player_free_units.
* AI adjusts satellite production amounts automatically based on population levels if it is below satellite_limit option.
* AI takes facility cost and maintenance now better into account in production choices.
* AI tries to build Geo Survey Pods in a way that they achieve better uniform coverage across territory.
* AI builds more colony pods to expand faster if there is a lot of space available.
* In the early game, AI tries to determine more accurately if it needs more scout units.
* Reduce crawler production slightly if the bases are already close together.


## Version 2.4 (2021-03-14)
* Add config options to choose if the Planet should randomly spawn expansion-only units. This works even if smac_only is not selected.
* Add many new config options for unit repair rates. This is the same implementation than in Will To Power.
* Unit repair rates have been reduced from standard values. In particular, Command Centers and similar facilities heal only 20% per turn.
* Bases in golden age are now highlighted with a colored label on the world map. This can be toggled with world_map_labels config option.
* Config options for social_ai have been expanded.
* Many general improvements to AI combat movement logic to take into account more factors.
* AI tries to avoid probing enemy factions that have the Hunter Seeker Algorithm.
* AI is updated to sometimes choose a wider variety of naval invasion locations.
* Fix issue where AI wasted Choppers because of fuel exhaustion.
* Fix bug with design_units that caused some AI prototypes to become corrupted in the savegame. The fix automatically prunes incorrect prototypes from the savegame while processing next turn change.


## Version 2.3 (2020-12-27)
* New option: cpu_idle_fix lowers the processing power consumption to a minimum by throttling unnecessary code. This uses the same method than smac-cpu-fix patch.
* New option: rare_supply_pods will reduce the frequency of supply pods to around one third of previous levels and instead places bonus resources on random maps. This option is a lesser version of "no unity scattering".
* New option: smooth_scrolling (disabled by default) adds support for some user interface enhancements provided by PlotinusRedux's PRACX mod.
* New option: windowed adds support to run the game using a user-supplied resolution. Command line option -windowed is also supported.
* Config file now includes options to set the display mode for the game. DirectDraw / DisableOpeningMovie options will override any values in Alpha Centauri.ini.
* By default the game starts now in native desktop resolution. Opening movie will also be skipped by default.
* Thinker now controls the movement of nearly all combat units, with the only major exception being missile units that are managed by the vanilla AI code.
* AI garrisoning priorities are rewritten to move defending units to bases that are the most threatened.
* Add AI planning capability to execute naval invasions over long distances with multiple transports using other combat ships as cover.
* Combat ships will now engage other targets with artillery much more often instead of always defaulting to normal direct attack.
* All artillery units will use new targeting logic to place them in more useful positions and prioritize threats more accurately.
* AI will occasionally use Drop Pods ability to deploy troops to other bases.
* Foil probe teams will now also attack coastal bases instead of only sea bases like previously.
* Changed territory_border_fix to increase the expansion of coastal base territory into ocean to an equal amount of actual sea bases.
* New feature: ALT+T shortcut displays general statistics about the factions.
* New feature: the game engine has a crash handler provided by Thinker that writes output to debug.txt if the game happens to crash for any reason.
* Set config option ignore_reactor_power enabled by default. This is non-vanilla but a much needed balance fix.
* AI tries to leave slightly more space between bases and prioritize locating bases on the ocean coastline.
* AI should build Tree Farms faster if the bases encounter eco damage unless the faction file has a negative PLANET modifier.
* AI will now attempt to raise land bridges between continents more often using better pathing. These goals are also stored in the save game.
* AI will occasionally disband formers or colony pods if there's no use for them.


## Version 2.2 (2020-11-15)
* New option: clean_minerals sets the amount of minerals a base can produce before experiencing eco damage.
* Improve rendering on ocean fungus tiles where they now display consistent graphics compared to the adjacent land fungus tiles.
* Game now displays a warning on startup if an unknown config option is detected in thinker.ini.
* Fix issue where revised_tech_cost was not working in the previous version.
* Fix engine bug where selecting bombardment on an empty square sometimes resulted in the attacking unit being destroyed.
* Fix very rare issue where a faction might get eliminated even when they had active colony pods.


## Version 2.1 (2020-11-08)
* New option: ignore_reactor_power (disabled by default). This makes the combat calculation ignore reactor power and treat every unit as having a fission reactor. Combat odds are also properly displayed in the confirmation dialog. More advanced reactors still provide their usual cost discounts. Planet busters are unaffected by this feature.
* New option: territory_border_fix will make the oldest bases claim tiles that are equidistant between two bases of different factions. Coastal bases also expand territory into the ocean for each of the 20 workable tiles in the base radius.
* Addition to eco_damage_fix: clean minerals cap will now increase every time Tree Farm, Hybrid Forest, Centauri Preserve or Temple of Planet is built even if no fungal pop has been encountered yet. It is not necessary anymore to delay building these facilities to alleviate eco damage.
* Faction placement is improved on custom maps where some starting locations were not previously picked.
* AI production planner tries to avoid building certain secret projects if they yield only marginal benefits.
* Include also TALENT and ROBUST modifiers in social_ai calculation.
* Fix issue where alien factions didn't gain the bonuses provided by free_colony_pods or free_formers.
* Fix issue where governors didn't sometimes work in player-owned bases.
* Fix AI vehicle home base reassignment bug where an apparent oversight made the game engine to reassign vehicles to bases with mineral_surplus < 2. Credits: Will To Power mod.


## Version 2.0 (2020-05-31)
* Road building algorithm has been completely rewritten to build more natural looking road networks.
* AI will now build magtubes after the tech is acquired.
* Draw a visual marker around the population label of HQ bases to identify them more easily.
* Tech costs in revised_tech_cost formula towards the late game are increased. Techs in the lower tiers are slightly cheaper than before.
* AI research bonuses in revised_tech_cost have been significantly reduced.
* Remove the mechanic where the first 10 techs are cheaper.
* Adjust the mod to calculate Social Engineering research effect similarly to vanilla game.
* AI prototype selection formula has been revamped to take into account more factors and to avoid selecting units that would take too long to produce.
* Adjust the AI logic whether to build conventional or PSI units. AI now takes into account if the faction has Worm Police ability.
* AI now attempts to design various units equipped with the Trance ability.
* AI sometimes builds Tachyon Fields, Geosynchronous Survey Pods and Flechette Defense Systems.
* Reduce overall sea level rise frequency to 1/3.
* Add Trance Scout Patrol and Police Garrison predefined units.
* Add proper parsing for special scenario rules: No terraforming, No colony pods can be built and No secret projects can be built.
* Add better handling for cases where the global unit count is nearing the limit (2048) and make the AI build facilities instead. If no suitable facilities are available, AI will switch production to Stockpile Energy.
* Add meaningful error message dialogs if some of the startup checks for Thinker fail.
* Import the following patches from Will To Power mod by tnevolin: collateral_damage_value, disable_planetpearls, disable_aquatic_bonus_minerals. Each of them also has their own config option.
* New option: auto_relocate_hq will automatically move lost HQs to another suitable base.
* New option: eco_damage_fix will equalize the eco damage between player and AI bases.
* New option: content_pop_player/content_pop_computer will adjust the default number of content population in each base.
* New option: free_colony_pods can be used to spawn extra units for the computer factions.
* Fix issue where AI might build Voice of Planet even if Transcendence victory condition was disabled.
* Fix issue where AI bases sometimes persist without any defenders. AI will now force switch production away from facilities if no valid defenders are present.
* Fix a bug where attacking other satellites didn't work in Orbital Attack View when smac_only was activated. Credits: Scient.
* Fix engine bug where a faction with enough energy credits would attempt to Corner Energy Market every turn. Now the AI properly only attempts it once and waits for the countdown to complete.


## Version 1.0 (2019-12-12)
* SMAC in SMACX mod files are now included with Thinker by default (can be enabled with smac_only=1)
* Removed smac_only dependency for ac_mod/labels.txt (redundant file for the mod)
* Factions with positive Planet rating will build less boreholes than the average
* Adjust tech_balance feature to also prioritize probe team prerequisite tech
* AI should relocate lost HQs faster than previously
* New option: revised_tech_cost provides an extensive overhaul for the research cost mechanics. More info in Details.md.
* New option: expansion_factor limits how many bases the AI will attempt to build before stopping expansion entirely
* New option: limit_project_start determines how soon the AIs are allowed to start secret projects
* Fix issue where AI sometimes ignored undefended enemy bases when it had the chance to capture them


## Version 0.9 (2019-04-07)
* Thinker now supports formers based on all triads: design_units will also create gravship formers when the techs are available
* AI formers build less roads and other smaller tweaks to former priorities
* AI sometimes nerve staples bases after UN charter has been repealed
* Military unit production priority is notably increased from previous amounts
* Whenever AI loses bases to conquest, this also triggers an extra priority to devote more resources to building new units
* Tech balance prioritizes more early economic techs to ensure important items are not skipped
* Rebalancing of social_ai logic to match faction priorities more closely
* Alphax.txt included in releases to provide optional changes
* Trance ability removed from design_units created probe teams
* Major adjustments to faction placement to improve starting locations
* Possibility to select custom factions in smac_only mod
* Added command line parameter "-smac" to start smac_only mod
* New option: nutrient_bonus for use with faction placement
* New option: hurry_items allows AI to use energy reserves to hurry production (excluding secret projects)
* New option: cost_factor allows one to change AI production bonuses
* New option: max_satellites specifies how many of them AI will build normally
* Fix spawning issues on Map of Planet
* Fix sea bases sometimes not building enough transports
* Fix colony pods freezing in place or being unable to reach their destinations
* Fix stuff being built on volcanoes
* Fix smac_only showing the expansion opening movie
* Fix custom prototypes having incomplete names


## Version 0.8 (2018-11-30)
* New combat logic for land-based units attempts to do counter attacks more often
* New code for selecting social engineering priorities
* AI understands the value of fungus and will sometimes plant it
* AI sometimes builds native units when the psi bonuses are high enough
* Faction placement option added: avoid bad spawns on tiny islands and equalize distances to neighbors
* Landmark selection options added to config
* Experimental load_expansion option added
* Fix secret project movies sometimes not being shown
* Scient patch updated to v2.0


## Version 0.7 (2018-10-03)
* Land-based former code rewritten
* Formers will attempt to raise land bridges to nearby islands
* Crawler code improved to choose better tiles
* Prototype picker values fast units and useful special abilities more
* Design_units configuration option added. Produces improved probe teams and AAA garrisons.
* AI produces now planet busters. Amount depends on the UN charter status and aggressiveness settings.
* AI prioritizes secret projects that match their interests
* AI builds secret projects now in their top third mineral output bases
* Tech requirements now fully moddable in alphax.txt
* Governors of all factions will use more borehole tiles
* Scient patch v1.0 added


## Version 0.6 (2018-08-18)
* First Github release
