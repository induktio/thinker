
# Thinker mod release changelog


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
