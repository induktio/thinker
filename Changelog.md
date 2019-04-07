
# Thinker mod release changelog
<!-- Full documentation available at: https://github.com/induktio/thinker -->


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
