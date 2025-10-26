
Main features
=============
Thinker has several advanced planning routines that enhance the base game AI to manage the complexities of colony building in Alpha Centauri. In the original game, many of the AI aspects were left underdeveloped while the computer factions struggled to deal with various punitive penalties. This left the single player experience severely lacking, since the AI would have no counter to many simple strategies.

Alpha Centauri was known for its immersive lore and story telling. As a general design principle, Thinker will attempt to improve gameplay mechanics and AI while leaving most of the lore as it is. Any instances where adding a feature or patch would be controversial with the original game narrative should be avoided, unless this is necessary for gameplay balance reasons. Hopefully this combination of changes will represent the original game accurately while also improving the gameplay experience and fixing many balance issues.

With minor exceptions, Thinker uses the same production bonuses as the original difficulty levels would grant the AI normally. There should be no extra resources received by the AI unless this is chosen in the configuration file. The main goal is to make the AI play better given any game config options, so generally the mod will not attempt to adjust original game design choices.

For the most complete list of all features provided by this mod, refer to both this file and `thinker.ini`. If something is not mentioned in these files, it is probably not implemented. Thinker Mod will affect many AI behaviors and also introduce some new game mechanics into Alpha Centauri. Generally most of the new features in the mod will have their own specific config options to adjust them. Items listed under "Other patches included" and "Scient's Patch" in this file will always be applied unless there is a config option listed to toggle them.

Thinker uses a compatible savegame format with the original game unless `modify_unit_limit` is enabled and the unit count exceeds the previous limit. In this case the savegame cannot be opened with the original game due to engine limitations. None of the other Thinker config options are preserved in the savegame either, but the units or resources spawned at the game start will remain.

Note that in `thinker.ini` config file, for binary settings **only zero values are treated as disabled**, any non-zero value usually enables the option. Whenever Thinker options are changed in the user interface, they are also saved to `thinker.ini` in the game folder.


Command line options
====================
    -smac       Activate SMAC-in-SMACX mod. Implies smac_only=1 config option.
    -native     Start game in fullscreen mode using the native desktop resolution.
    -screen     Start game in fullscreen mode using the resolution set in mod config.
    -windowed   Start game in borderless windowed mode using the resolution set in mod config.


Keyboard shortcuts
==================
    Alt+T       Show Thinker's version and options menu (use Alt+H when PRACX is enabled).
    Alt+R       Toggle tile information update under cursor if smooth_scrolling is enabled.
    Alt+O       Enter a new value for random map generator when scenario editor mode is active.
    Alt+L       Show endgame replay screen when scenario editor mode is active.
    Alt+Enter   Switch between fullscreen and windowed mode unless desktop resolution is used in fullscreen.
    Ctrl+H      Hurry current production with energy reserves when the base window is active.


User interface additions
========================
Thinker's in-game menu shows the mod version information and provides statistics about the factions and spent game time if a game is loaded. In addition, some Thinker config options can be adjusted from its sub menu. When the game is in the main menu, it is also possible to adjust Special Scenario Rules for new random maps started from there. Selecting any special rules will override all rules even when starting a new scenario, so leave all the choices in the dialog empty to use preset scenario rules.

Statistics feature calculates mineral and energy production numbers after multiplier facility effects are applied. However the mineral output bonus provided by Space Elevator is ignored in this step. Energy calculation also does not subtract inefficiency from the final number.

Render base info feature draws colored labels around various bases to identify them more easily and shows more details on the world map. HQ bases are highlighted with a white label. Player-owned bases that are about to drone riot or enter the golden age are highlighted with colored labels. Bases that have Flechette Defense System or Geo Survey Pods are highlighted with a blue label.

Base window will also show additional details that were previously not directly visible on the user interface. Any satellite production bonuses are also shown in the base resource view. When the base is producing Stockpile Energy, the extra credits gained per turn is shown in the base resource view. It is also possible to switch between base window tabs by pressing `Ctrl+Left` or `Ctrl+Right`.

The game will show population counts for talents, workers, drones and specialists. Bases that are about to drone riot, enter the golden age or pop boom on the next turn are highlighted with colored labels. When a base has been nerve stapled, the remaining turns for the staple effect are shown on the bottom right corner.

Ecological damage display on the base window has been updated to be more useful. Whenever the mineral production level is sustainable on planet's ecology, the Eco-Damage percentage is displayed in green indicating how much of the clean minerals capacity is in use. Whenever this number is exceeded and the base begins to cause ecological disruption, the percentage is displayed in red and the number over 100% shows the current Eco-Damage being experienced at the base. This will not change how [ecological damage](https://web.archive.org/web/20220310074913/https://alphacentauri2.info/wiki/Ecology_(Revised)) is calculated by the game unless minor details are modified by `eco_damage_fix` option.

![Base support window](/tools/support_view.png "Base support window is zoomable with the mousewheel and shows nearby supported units by the current base.")

Whenever the player instructs a former to build an improvement that replaces any other item in the tile, the game will display a warning dialog. This dialog can be skipped by toggling `warn_on_former_replace` option.

To speed up the gameplay several user interface popups have been moved into delayed notifications that are also listed on the message log. These notifications include satellite completion by any faction, forest/kelp growing near player bases and unit promotions after combat.

When building or capturing a new base, the mod will automatically copy the saved build queue from **Template 1** to the new base. At maximum 8 items can be saved to the template, and the first item from the template will be automatically moved to current production choice when a new base is built or captured. Only the template saved to the first slot is checked, any other saved templates are ignored.

To save the current queue to template, open a base and **right click** on the queue and select **Save current list to template**. This feature works in conjunction with the simple hurry cost option so that it's easy to start hurrying base production on the first turn without worrying about double cost mineral thresholds.


Summary of AI changes
=====================
Thinker introduces notable changes to the AI to manage the colonization and production much more effectively than usual which can result in changes on the difficulty curve. When playing on lower difficulties, it might make sense to apply additional limits on AI expansion by using options such as `expansion_limit=10` and `expansion_autoscale=1` so that the AI will expand roughly at the same speed as the player faction.

Thinker fully controls the movement of most units, combat and non-combat alike. For testing purposes it is also possible to run old/new AIs side-by-side. For example `factions_enabled=3` makes only the factions in the first 3 slots to use Thinker AI if they are not player-controlled. By default `factions_enabled=7` setting enables Thinker AI for all computer factions. Regardless of which AI is enabled, all of them should receive the same production bonuses that are set in the config file.

Another novel addition has been naval invasions executed by the AI. This has been traditionally a weak spot for many AIs, since they lack the coordination between many units to pull off such strategies. Thinker however is capable of gathering an invasion fleet with many transports and using other combat ships as cover to move them into a landing zone. Combat ships will also use artillery attack on various targets much more than usual.

Thinker prioritizes naval invasions if the closest enemy is located on another continent. Otherwise, most of the focus is spent on building land and air units. At any given time, only one priority landing zone can be active for the AI. Improved combat routines for land-based units will attempt to counter-attack nearby enemy units more often. If the odds are good enough, hasty attacks are executed more often than usual. The AI will fight the native units more aggressively, and it will also try to heal its units at monoliths.

Base garrisoning priorities are also handled entirely by Thinker which tries to prioritize vulnerable border bases much more than usual. Air units also utilize the same priorities when deciding where to rebase. You might notice there's less massive AI stacks being rebased around for no meaningful reason. Instead Thinker tries to rebase the aircraft in much smaller stacks to more bases so that they can cover a larger area.

Thinker base production AI will also decide every item that is produced in a base. The build order can differ substantially from anything the normal AIs might decide to produce, and the difference can be easily noted in the vastly increased quantity of formers and crawlers the AIs might have. Design units feature will introduce custom probe teams, armored crawlers, gravship formers, and AAA garrison prototypes for the AI factions. Normally these prototypes are not created by the game engine.

Base hurry feature is able to use AI energy reserves to occasionally hurry base production. Items that can be hurried include most basic facilities, formers, combat units, and sometimes secret projects. The amount of credits spent on rushing projects depends on difficulty level. When a project has been rushed, it will be displayed in a popup if the player has a commlink for the faction. This works even during sunspots.

Social AI feature will decide the social engineering models the AI factions will choose. It will attempt to take into account the various cumulative/non-linear effects of the society models and any bonuses provided by the secret projects. The AI is now capable of pop-booming if enough growth is attainable, and it will also try to avoid pacifist drones by switching out of SE models with too many police penalties. All the SE model effects are moddable because the AI is not hardcoded in any particular choices. This feature is also capable of managing all the custom factions.

Tech balance feature will prioritize certain important techs when choosing blind research targets, such as the requirements for formers, crawlers, recycling tanks, children's creches, recreation commons, and the 3 technologies to allow the production of more than 2 resources per square. If these techs are not available for research, the tech progression is unaffacted by this feature.


Diplomacy changes
=================
For the most part, AI factions utilize original game logic when deciding on any actions having to do with diplomacy. There are some notable exceptions where Thinker patches existing diplomacy dialogue and decision making.

Energy loan dialogue ("generous schedule of loan payments") has been revamped to make better decisions on what kind of loans to grant. AI willingness for loan granting is now heavily dependent on treaty status, diplomatic friction, integrity blemishes, and the relative size of the factions (more powerful opponent factions are disliked). AI also has new dialogue when they reject a proposed loan.

Base swapping dialogue has been adjusted to reject any base swaps where the AI would previously accept very disadvantageous trades for itself. AI can be willing to exchange bases for energy credits or another base that is worth more according to the same valuation. During multiplayer games base swapping is disabled altogether, unless the AI faction is an ally and has surrendered previously.

Tech trading for energy credits has been modified such that AIs will not offer to buy techs if they do not have the necessary credits available. When techs are traded for credits, AIs will use a new tech valuation scheme that makes it more expensive to buy later techs. Many similar factors as used in the energy loan dialogue affect how good the offered tech trades are. Techs that are known by only one faction are the most valued in trading and they are adjusted by the following modifier.

* 1 owner: 100% value
* 2 owners: 70% value
* 3 owners: 55% value
* 4 owners: 40% value
* 5 or more owners: 25% value


Player automation features
==========================
It is possible to instruct Thinker to automate player-owned units from `ALT+T` menu options. There's also a separate option to set Thinker manage governors in player-owned bases. Normally player base governors and unit automation would be handled by the original AI. However worker tile selection and specialist allocation is always managed by Thinker.

When enabled, use `Shift+A` shortcut to automate any specific unit and it will follow the same colonization or terraforming strategy as the AI factions. Colony pods will attempt to travel to a suitable location autonomously and deploy a base there. If this is not required, the units can be moved manually as well. Automated formers should follow the same settings listed in the Automation Preferences dialog. Crawlers will usually try to convoy nutrient or mineral resources depending on the base status.

Beware formers automated in this way **can replace any existing improvements** on the territory if Thinker calculates the replacement would increase tile production. A notable difference compared to the AI factions is that player-automated formers will never raise/lower terrain. To prevent the formers from ever replacing some specific improvements, such as bunkers, they need to be placed outside the workable base radius from any friendly base.

Bases managed by Thinker governors will mostly follow the same options as provided in the base governor settings. Adjusting explore/discover/build/conquer governor priorities from the base window has the same effect compared to AIs using these choices as faction priorities. Enabling terraformer production in the governor settings will also allow governors to build crawlers since they don't have a separate option in the menu.

By default Thinker governors will not attempt to start secret projects or hurry production with energy reserves, but this can be enabled from the governor settings. If production is hurried by the base governor, this will be displayed as a separate line in the message log. Normally the governor tries to keep some energy credits in reserve before attempting to hurry production.

Governor is also able to choose between multiple specialist types based on social engineering choices and various conditions. If the base has Build/Conquer priorities selected, this will guide the governor to maximize mineral output even if some specialists could be useful for the base. If the governor has to reallocate many drones into specialists, this will be indicated with PSYCHREQUEST message if increasing psych spending could be potentially useful.


Map generator options
=====================
![Map generator comparison](/tools/map_generators.png "Map generator comparison using average ocean coverage and average erosion.")

Using the default settings, the game's random map generator produces bland-looking maps that also have way too many small islands that make the navigation much harder for the AI. As an optional feature, Thinker includes a rewritten map generator that replaces the old WorldBuilder. It produces bigger continents, less tiny islands, more rivers and ocean shelf using more natural layouts. This can be toggled with `new_world_builder` config option or from Thinker's `ALT+T` menu options.

This feature supports all the world settings chosen from the game menus such as map size, ocean coverage, erosion, natives and rainfall level. With Thinker's version, it is possible to set the average ocean coverage level with `world_sea_levels` option, and this will provide a consistent way of setting the ocean amounts for any random maps. Changing this variable has a very high impact on the generated maps. Lower numbers are almost always pangaea maps while higher numbers tend towards archipelago layouts. There is no simple way of accurately doing this with the default map generator.

From the menu options it is also possible to choose the random map style from larger or smaller continents. Note that this has only significant impact if the ocean coverage is at least average, since at low levels all generated maps will tend towards pangaea layouts.

All landmarks that are placed on random maps can also be configured from `thinker.ini`. Nessus Canyon is available but disabled by default. When new map generator is enabled, `modified_landmarks` option replaces the default Monsoon Jungle landmark with multiple smaller jungles dispersed across the equator area. Planet rainfall level will determine how many jungle tiles are placed. Borehole Cluster is also expanded to four tiles and Fossil Field Ridge is placed on ocean shelf tiles.

It is also possible to place multiple similar landmarks if the corresponding landmark option is set to some value greater than one. When playing on smaller maps, it might make more sense to disable some additional landmarks, as otherwise the map generator may skip placing some landmarks due to not having enough space.

The new map generator is entirely different from the original version, so it does not use most variables specified in `alphax.txt` WorldBuilder section. Only in limited cases, such as river or fungus placement, WorldBuilder variables might be used by the game engine when the new map generator is enabled.

Thinker also includes modified `faction_placement` algorithm that tries to balance faction starting locations more evenly across the whole map area while avoiding unusable spawns on tiny islands. The selection also takes into account land quality near the spawn. The effect is most noticeable on Huge map sizes. It is possible to disable this option but it is not recommended since other spawn-related features might depend on it.

When `nutrient_bonus` option is higher than zero, the placement algorithm adds this many additional nutrient bonus resources at each start. The placement also strongly favors spawns on river tiles for easier movement in the early game.

Another optional setting `rare_supply_pods` is not dependent on faction placement, instead it affects the whole map by reducing the frequency of random supply pods significantly. Thematically it never made much sense that the supply pods would be excessively abundant across the whole planet surface, while the Unity spaceship would supposedly only have limited space for extra supplies.

When `rare_supply_pods` option is enabled, supply pods are also less likely to have events that rush production at the nearest base when it needs more than 40 minerals. The rushed production amount will always have to be less than 100 minerals or otherwise the result is changed to something else.


Accelerated start feature
=========================
Accelerated start option will use a modified initial start when `time_warp_mod` is enabled. Previously the game auto-generated additional bases with random improvements. In this version each faction starts with one HQ base and much more additional units, including colony pods, formers and scout units. The amount of starting units and credits depends on the map size, with more being granted on larger maps.

Time Warp should give each faction an equal number of starting colony pods, even if the faction is Alien or Aquatic. The amount of techs being granted or receiving an initial secret project can be adjusted from config. Keep in mind that any additional techs will increase later research costs in the default tech cost rules. The starting turn of the game can also be adjusted and this will affect various game mechanics, such as planet-owned native unit default lifecycle level.

In the nascent days of Alpha Centauri colonization, pioneers faced an arid, unforgiving landscape. Determined to thrive, they started planting forests and cultivating kelp farms. Through steady effort, they transformed the desolation into a thriving oasis. Terraformers worked meticulously to remove fungus from nearby important bonus resources, and this fueled more growth on the initial settlement. After years of studying the local ecosystem and building industrial capacity, it is now time to go for more expansion.


Improved combat mechanics
=========================
![Long range artillery](/tools/ship_artillery.png "Long range artillery option allows the special ability to grant longer range for ship-based weapons.")

In Alpha Centauri, Fusion reactor technology was extremely important for military purposes as it cheapens the reactor cost almost by half and doubles the unit's combat strength. This made the tech extremely unbalanced for most purposes, and there's no good way around this if the tech tree is to have any advanced reactors at all. By default Thinker's `ignore_reactor_power` option keeps all reactors available, but ignores their power for combat calculation purposes. More advanced reactors still provide their usual cost discounts and also planet busters are unaffacted by this feature.

When `ignore_reactor_power` option is enabled, it is recommended to adjust some game preferences to reduce automated prototype additions when new techs are discovered.
Recommended option to disable is `Design units automatically` which affects some but not all automated designs.
Another option to disable is `Auto-prune obsolete units` which is triggered whenever the faction discovers a new reactor tech, and this can result in dozens of repetitive popups.
Note that even if the original user interface shows the auto-prune option as recommended, the dozens of generated dialogs with each reactor tech can be an obstacle for smooth gameplay.

Another notable change is the introduction of reduced unit healing rates. In the original game, units were often able to fully heal in a single turn inside a base but this is no longer the case. Prolonged offensives are no longer trivially easy against opponents since units may have to stop healing for multiple turns. The healing rate can be significantly increased by building Command Centers and other similar base facilities.

Previously Choppers were the most important air unit due to their fast attack rate. To balance air units, the mod changes each Chopper attack to expend two movement points instead of only one like previously. Chopper range is also reduced to be slightly less than Needlejet range. Ranges for all air units can still be increased by using better reactors, building Cloudbase Academy or including Antigrav Struts/Full Nanocells special abilities.

Related to combat mechanics, it is also possible to adjust movement speeds on magtubes with `magtube_movement_rate` option. By default this option allows zero cost movement on magtubes but it can be changed for example to allow twice as fast movement compared to normal roads.
Setting magtube movement rate higher than 3 is not recommended because it may cause variable overflow issues for fast aircraft units. In this case the game will limit the maximum speed allowed for such units.

When bases are captured from another faction, the captured base extra drone psych effect is shown separately on the psych window. Previously this period was always 50 turns while in the modded version this depends on the population size and facilities in the captured base, and it can range from 20 to 50 turns. This makes it somewhat easier to assimilate smaller bases while larger bases will still take a long time to switch previous owner.

In the original game, combat morale calculation contained numerous bugs and contradictions with the game manual. These issues particularly with Children's Creche and Brood Pit are further described in [this article](https://web.archive.org/web/20201016221232/https://alphacentauri2.info/wiki/Morale_(Advanced)).

In the patched version enabled by default, units in the headquarters base automatically gain +1 morale when defending as mentioned in the manual.
For non-native units, when the home base is experiencing drone riots, this always applies -1 morale unless the faction has a special rule for ignoring negative morale modifiers.
For independent units drone riots have no effect.

When the unit involved in combat is inside the base tile, Children's Creche grants +1 morale for all units and cancels penalties from negative SE morale rating for non-native units.
For native units inside the base tile and in the absence of Creche, Brood Pit grants +1 lifecycle. This effect does not stack with Creche and has no effect on non-native units.
Neither Creche nor Brood Pit affects combat outside the base tile even if the unit's home base contains such facilities.


AI production bonuses
=====================
This list is not complete, but it details the most important bonuses granted for AI factions, thus changing these settings has the largest impact on AI performance. There can be various other, smaller benefits granted for the AIs by the game that are not listed here.

1. AI mineral production and nutrient base growth cost factors for each difficulty level can be changed with `cost_factor` setting. Does not affect other difficulty related modifiers.
2. When `revised_tech_cost` is enabled, `tech_cost_factor` scales the AI tech cost for each difficulty level.
3. Content (non-drone) base population for each difficulty level can be adjusted with `content_pop_player` and `content_pop_computer` variables.
4. AI pays reduced maintenance for facilities on two highest difficulty levels. Transcend level has 1/3 and Thinker level 2/3 maintenance from the normal amounts.
5. AI pays no retooling costs whenever it changes production from one item to another.


Difficulty level effects
========================
This list details the most important changes on different difficulty levels for player factions in the original game.
For AI factions, difficulty in some calculations is treated as a fixed value. Any changes made by the mod are mentioned separately.

Easier Diplomacy. At lower levels, computer players will be more likely to "go easy" on players in diplomacy, unless the "Intense Rivalry" option is enabled, in which case they are much more ruthless.

Delayed Gang Tackles. In particular, the tendency for computer players to "gang up on" the human player is controlled by the difficulty level and current turn thresholds. With Intense Rivalry enabled, difficulty is treated as Transcend.

Also computer players will not gang up until the player's overall "dominance bar" is significantly higher than the dominance bar of the second-place player: it need only be 20% higher with Intense Rivalry, but otherwise it needs to be 50% higher at Librarian, Thinker and Transcend levels, twice as high at Talent level, and never happens at Citizen and Specialist levels.

No Mind Control. At Citizen, Specialist and Talent levels, computer players will not use Mind Control against player bases.

Secret Projects. At Talent and below, the other factions can't start work on a Secret Project until the player has its prerequisite tech, even if they already have the tech in question. This can be adjusted with `limit_project_start` option.

Colony Pod. At Citizen and Specialist levels, building a colony pod at a size 1 base does not eliminate the base.

No Early Research. At Citizen and Specialist levels, all research points accumulated during the first 5 turns are ignored (removed by early_research_start).

Command Center Maintenance. The Maintenance cost of the Command Center facility is equal to the best Reactor level that is available (1 to 4), but never more than half of DIFF rounded up. In the mod Command Center only uses the maintenance cost set in alphax.txt.

No Power Overloads. At Citizen and Specialist levels. Base facilities do not experience power overloads when running out of energy to pay maintenance.

No Pop Lost to Attack. At Citizen level, the bases never lose population points when they are attacked.

Ecological Damage. AI bases will not trigger sea level rise events unless the difficulty is Thinker or Transcend.

Random Events do not occur before Turn `75 - (DIFF * 10)`.

No Prototype Cost. At Citizen and Specialist levels, players do not have to pay for "prototypes" of units.

No Retooling Penalty. At Citizen and Specialist levels, there is no penalty for switching production in progress at a base.

Cost to Change Society. The cost to change social engineering is affected by the difficulty level:

    CHANGE = (the number of social areas changed in a turn) + 1

    Upheaval Cost = CHANGE * CHANGE * CHANGE * DIFF


Hurry and upgrade formulas
==========================
![Hurry dialog](/tools/hurry_dialog.png "Hurry dialog displays the minimum cost to complete production on the next turn.")

To ease calculations, `simple_hurry_cost` option removes the double cost penalty from production hurrying when the accumulated minerals are less than the Retool penalty set in alphax.txt. Otherwise the hurry cost formula is equal to the vanilla game mechanics.

In both cases, rushing facilities cost 2 credits per mineral and secret projects cost 4 credits per mineral. In addition secret projects have a second double cost penalty if the accumulated minerals are less than 40.

Base window hurry dialog will now display the minimum required hurry cost to complete the item on the next turn. This assumes the mineral surplus does not decrease, so take it into account when adjusting the workers. When entering a partial payment, this minimal amount will be the default choice in the dialog, instead of the full hurry cost like previously.

All crawler units have a special ability that enables them to disband their full mineral value towards secret projects, while other units are able to disband only half of their mineral value on production.
Optional feature `modify_upgrade_cost` changes the upgrade cost for crawlers such that it is equal to the mineral row cost difference between two prototypes multiplied by 40 while for non-crawler units the difference is multiplied by 20.
If the difference is zero or negative, the value is treated as being one mineral row. In addition the upgrade cost is at least 10 credits for each mineral row in the new unit regardless of the difference.
This means crawlers can be used to rush projects like previously, but it will confer no advantage beyond bypassing the initial 40 mineral double cost threshold.


Revised tech costs
==================
In the original game, research costs were mainly decided by how many techs a faction had already researched. That meant the current research target was irrelevant as far as the costs were concerned. This is somewhat counter-intuitive since lower level techs would be worth the same than higher level techs. For speed runs, it made sense to avoid researching any techs that were not strictly necessary to keep the cost of discovering new techs as low as possible.

The config option `revised_tech_cost` attempts to remake this mechanic so that the research cost for any particular tech is mostly fixed and depends on the level of the tech. This follows the game design choices that were also made in later Civilization games. Enabling this feature should delay the tech race in mid to late game. This formula still uses some adjustments for cheaper early game research and adds a minor modifier for the number of known techs by the faction.

For example, in the default tech tree, Social Psych is level 1 and Transcendent Thought is level 16. See also a helpful chart of [the standard tech tree](https://www.dropbox.com/sh/qsps5bhz8v020o9/AAAkyzALX76aWAOc363a7mgpa/resources?dl=0&lst=). The base cost for any particular tech is determined by this formula.

    5 * Level^3 + 25 * Level^2 + 20 * KnownTechs

Here are the base costs without tech count adjustment for standard maps.

| Level | Labs  |
|-------|-------|
|     1 |    30 |
|     2 |   140 |
|     3 |   360 |
|     4 |   720 |
|     5 |  1250 |
|     6 |  1980 |
|     7 |  2940 |
|     8 |  4160 |
|     9 |  5670 |
|    10 |  7500 |
|    11 |  9680 |
|    12 | 12240 |
|    13 | 15210 |
|    14 | 18620 |
|    15 | 22500 |
|    16 | 26880 |

The idea here is that level 1-3 costs stay relatively modest and the big cost increases should begin from level 4 onwards.
This feature is also designed to work with `counter_espionage` option. Keeping the infiltration active can provide notable discounts for researching new techs if they can't be acquired by using probe teams.

After calculating the base cost, it is multiplied by all of the following factors.

* The first 10 techs discovered by the faction are discounted with the modifier decreasing for every new tech gained. Every faction starting tech counts against this limit.
* For AI factions, `tech_cost_factor` scales the cost for each difficulty level, e.g. setting value 84 equals 84% of human cost.
* Multiply by the square root of the map size divided by the square root of a standard map size (56).
* Multiply by faction specific TECHCOST modifier (higher values means slower progress).
* Divide by Technology Discovery Rate set in alphax.txt (higher values means faster progress).
* If Tech Stagnation is enabled, the cost is doubled unless a different rate is set in the options.
* Count every other faction with commlink to the current faction who has the tech, while allied or infiltrated factions count twice. Discount 5% for every point in this value. Maximum allowed discount from this step is 30%. Only infiltration gained by probe or pact counts for the purposes of extra discount, the Empath Guild or Planetary Governor status is ignored in this step.

The final cost calculated by this formula is visible in the F2 status screen after the label "TECH COST". Note that the social engineering RESEARCH rating does not affect this number. Instead it changes "TECH PER TURN" value displayed on the same screen.


Expiring infiltration feature
=============================
Normally establishing infiltration with a probe team on another faction is permanent and cannot be removed in any way. In multiplayer games this can be especially unbalanced. Thinker provides a config option `counter_espionage` to make infiltration expire after a specific period based on a variety of factors.

When enabled, every time a probe teams infiltrates a base of another faction, a popup will display the amount of turns infiltration is expected to last. Infiltration should always succeed, however the probe team may be randomly lost afterwards, as in vanilla game rules. Whenever infiltration is discovered and removed, both factions will receive a notification on their own turns. While the spying faction is the governor or has the Empath Guild, infiltration will never expire regardless.

The infiltration status can be renewed once per turn for every opponent faction which resets the expiration counter to the initial value. The duration is determined by comparing espionage ratings of two factions. In this mechanic, it is important to have a better PROBE/POLICE rating or at least a parity to maintain the infiltration for longer periods of time.

* Current social engineering PROBE rating counts twice.
* Initial PROBE rating in faction definition counts once.
* Current POLICE rating counts once.
* Each tech that increases probe team base morale counts twice.

Each social engineering value is treated as being from -3 to 3 even if it falls outside this range. In addition, infiltration lasts longer on maps larger than the standard size to balance for the longer travel distances. On Thinker and Transcend difficulty levels, the duration is reduced from normal values. The final time value is always at least 5 turns. For example, on Transcend difficulty, standard map, with two factions having equal ratings, the default infiltration period is 15 turns.


Free base facilities
====================
By default alphax.txt files contain entries for each facility labeled as "Free = Technology for free facility at new bases" some of which are enabled.
These values were not parsed by the original game even though they were referenced in code that adds these facilities for free at newly built bases when the faction has discovered the specific technologies.
This does not exclude facility maintenance costs unless the faction has these facilities granted for free in their faction definition.

The modded version parses these entries from alphax.txt and adds the facilities on new bases if the technology is discovered.
This feature can be always disabled by setting the free technologies to `Disable`. Recycling Tanks is free after discovery of Advanced Ecological Engineering (B7) and Recreation Commons is free after Sentient Econometrics (E11).
The original game also included entries for free Network Node after Self-Aware Machines (D11) and Energy Bank after Quantum Machinery (B12) but these entries are disabled to limit the benefits received by building additional bases.


Base psych changes
==================
In the original game benefits provided by Paradise Garden, Human Genome Project and Clinical Immortality sometimes did not match their description in the manual. Often numerous drones or superdrones added by bureaucracy prevented new talents from being added on the base. In the modified routines enabled by default with `base_psych` option, these facilities provide more consistent effects. Clinical Immortality provides one extra talent per base as mentioned in the manual.
It is also possible to modify or restore the original effects for each of them by adjusting `facility_talent_value` option. If the base has many superdrones, these facilities and projects will suppress more drones than in the original game if the additional talents are not added directly.

As an additional feature, psych energy allocation is not limited to twice the base size unlike in the original game. Any spending before this limit will provide similar effects as in the original game, but after that limit the marginal cost to pacify one drone or create more talents is increased to 4 psych and it keeps increasing by 4 psych after each step.
This makes it slightly easier for some factions to achieve golden ages while it still requires substantial psych spending. Any other psych effects except this allocation increase or talents modified by `facility_talent_value` remain the same as before.


Recommended alphax.txt settings
===============================
All alphax.txt files provided by Thinker contain some optional, minor edits as summarized below.
These edits will introduce fairly minimal differences to the vanilla game mechanics and they also will not change the default tech tree ordering.

1. Fix: Enabled the "Antigrav Struts" special ability for air units as stated in the manual.
2. Fix: Disabled the "Clean Reactor" special ability for Probe Teams because they already don't require any support.
3. The frequency of climate change effects is reduced. Thinker level AIs regularly achieve much higher mineral production and eco damage than the AIs in the base game. As a result, excessive sea level rise may occur early on unless this is modified.
4. WorldBuilder is modified to produce bigger continents, less tiny islands, more rivers and ocean shelf on random maps.
5. Large map size is increased to 50x100. Previous value was nearly the same as a standard map.
6. Descriptions of various configuration values have been updated to reflect their actual meaning.
7. Add new predefined units Trance Infantry, Police Garrison and Drop Colony Pod.
8. Plasma Shard weapon strength is raised to 14.
9. Secret project priorities are adjusted for the AI to start important projects first.
10. Chopper range is reduced to 8 (fission reactor).
11. Missile range is increased to 18 (fission reactor).
12. Crawler, Needlejet, Chopper and Gravship chassis costs are slightly increased overall. This results in about 10 mineral increase for most prototypes. For example the default Supply Crawler prototype costs 40 minerals.
13. When using SMAC in SMACX mod, the documentation displayed in the game will contain additional information about game engine limitations, and many bugs are marked with KNOWN BUG tag.
14. The amount of techs that grant automatic morale boosts for probe teams is reduced from five to two. These techs are now Pre-Sentient Algorithms and Digital Sentience. This makes it much harder to build elite-level probe teams.
15. Cost factor for Artillery special ability is changed to "increases with speed value" instead of "armor+speed value". It is also possible to add Artillery on ships to increase their range when `long_range_artillery` is enabled.


SMAC in SMACX mod
=================
Thinker includes the files necessary to play a game similar to the original SMAC while disabling all expansion-related content. See the original release posts of SMAC in SMACX [here](https://github.com/DrazharLn/smac-in-smax/) and [here](https://web.archive.org/web/20220812201356/https://alphacentauri2.info/index.php?topic=17869.0).

Thinker also adds support for custom factions that are not aliens while using this feature. Installing this mod doesn't require any extra steps since the files are bundled in the zip file.

1. Download the latest version that includes `smac_mod` folder in the zip.
2. Extract all files to the Alpha Centauri game folder.
3. Open `thinker.ini` and change the configuration to `smac_only=1` ***OR*** start the game with command line parameter `thinker.exe -smac`
4. When smac_only is started properly, the main menu will change color to blue as it was in the original SMAC, instead of the SMACX green color scheme.
5. The game reads these files from `smac_mod` folder while smac_only is enabled: alphax.txt, helpx.txt, conceptsx.txt, tutor.txt. Therefore it is possible to keep the mod installed without overwriting any of the files from the base game.


Installing custom factions
==========================
First any downloaded faction txt and pcx files have to be extracted to the main game folder. There should not be any need to overwrite files unless replacing factions in the base game.
Note that even when using smac_only mode, the factions have to be extracted to the main game folder since the same files are used in both game modes. The faction files must not be placed in any subfolder under the main game folder.

To install the factions on the game startup menu, they need to be added in `alphax.txt` in the default expansion or `smac_mod/alphax.txt` when using smac_only mode.
Take note of the required faction definition .txt files and add them into the `#CUSTOMFACTIONS` section without .txt suffix. For example, to enable the two hidden factions in the base game, edit the section like below.

    #CUSTOMFACTIONS
    BRIAN, BRIAN
    SID, SID

At most 10 custom factions can be listed in `#CUSTOMFACTIONS` because of limitations in the user interface. See below for some custom faction sets. The factions can be played in both game modes and they are also fairly balanced overall.

* [Sigma Faction Set](https://www.dropbox.com/sh/qsps5bhz8v020o9/AAAG16eQBuaX45fGFENKWq48a/factions/Sigma_Faction_Set_2014-06-19.zip?dl=0)
* [Dystopian Faction Set](https://www.dropbox.com/sh/qsps5bhz8v020o9/AAA0kNw0tk5sESLZQ3eDMEeFa/factions/Tayta_Custom_Factions_v4.zip?dl=0)
* [Network Node Faction Set](https://www.dropbox.com/sh/qsps5bhz8v020o9/AACyzRqJ6ULvneh2Iz-0KPvZa/factions/NetworkNodeFactions.zip?dl=0)


Compatibility with other mods
=============================
Custom programs can be used to play game movies if they have support for the appropriate wve format.
[VLC](https://www.videolan.org/vlc/) is the recommended program for this and at startup the mod will attempt to detect if it is installed in the default folder.
If it is not found, custom path can be set in `MoviePlayerPath` and command line options in `MoviePlayerArgs` in Alpha Centauri.ini. If these options are left empty, the game uses the default video player.
If the lines are removed, the mod will reset to the default VLC installation path and options shown below when the game is restarted. Note that Thinker's video player options always override any other movie player settings used by PRACX.

    C:\Program Files\VideoLAN\VLC\vlc.exe --fullscreen --video-on-top --play-and-exit --no-repeat --swscale-mode=2

It should be possible to run both Thinker and [PRACX](https://github.com/DrazharLn/pracx) graphics enhancement patch at the same time. For easiest installation, download [version 1.11 or later](https://github.com/DrazharLn/pracx/releases/). However this combination of patches will not receive as much testing as the normal configuration, so some issues might occur.

If any issues are encountered, first check if they occur with the original game and/or Thinker without additional mods. Also some optional features provided by Thinker will be disabled while running PRACX because they would patch conflicting areas of the game binary. These disabled feature include:

* Video mode and window size config options.
* Smooth scrolling config option.

Scient's patch is already included in the Thinker binary and does not need any additional installation steps from the user. Optionally it is possible to install the modified txt files provided by Scient's patch. 
Any other mod that uses a custom patched binary not listed here is not supported while running Thinker. Using any other binaries may prevent the game from starting if any of Thinker's startup checks fail because of an incompatible version.


Known limitations
=================
Currently the features listed here may not be fully supported or may have issues while Thinker is enabled. The list may be subject to change in future releases.

1. Network multiplayer (TCP/IP) should be supported, however this is less stable than the singleplayer configuration, so some issues might occur. At minimum every client needs to use the same configuration files. In case of network problems, refer to other manuals on how to configure firewalls and open the necessary ports for multiplayer. Simultaneous turns option for network multiplayer is known to cause issues. If the game desyncs, it is possible to save it to a file and host it again.
2. Most notable limits in the game engine are 8 factions with one for native life, 512 bases, 64 prototypes for each faction, and 2048 units unless increased by options. These limits were fixed in the game binary at compilation time and the faction limit may not be feasible to increase without substantial rewrites.
3. Faction selection dialog in the game setup is limited to showing only the first 24 factions even if installed factions in alphax.txt exceed this number.
4. Most custom settings in "Special Scenario Rules" should be supported even when starting random maps. However these rules may cause inconsistent behavior when used from the main menu: `Force current difficulty level` and `Force player to play current faction`.
5. DirectDraw mode is not supported while using thinker.exe launcher. In this case the game may fail to start properly unless `DirectDraw=0` config option is used.
6. Upgrading any units to predefined prototypes in alphax.txt is not supported due to game engine limitations. It is only possible to upgrade units to prototypes defined for the specific faction.
7. In rare cases needlejets that are set to automated air defense may disappear after ending their turn outside the base. This should not happen if the units are moved manually.
8. Player controlled units set to automated orders may sometimes discard their automation status and request new orders from the player. There is no consistent workaround for this other than giving additional orders.
9. Security Nexus (F7 hotkey) units active/queue counters will wrap around when they reach 256 units starting from zero again. This is mostly an issue with the user interface since any upgrade calculations will use the correct unit counts.
10. When SMAC-in-SMACX mod is active, Borehole Cluster and Manifold Nexus do not trigger interludes that were present on SMAC when entering specific landmark tiles on the map since these interludes were removed from the expansion code.


Other patches included
======================
This list covers the game engine patches used by Thinker that are not included in Scient's Patch. These patches are all enabled by default.
If the line mentions a config variable name in parentheses, the patch can be optionally disabled by adding `config_variable=0` in thinker.ini.

1. Modify governor worker allocation to better take into account any psych-related issues and convert workers into specialists as needed. Worker tile selection is improved to emphasize high-yielding tiles such as boreholes.
2. Governor is able to choose from multiple specialist types based on social engineering choices and various factors. The default specialist type chosen by the game has less emphasis on psych compared to the original version.
3. Fix issue where attacking other satellites doesn't work in Orbital Attack View when smac_only is activated.
4. Fix engine rendering issue where ocean fungus tiles displayed inconsistent graphics compared to the adjacent land fungus tiles.
5. Fix game showing redundant "rainfall patterns have been altered" messages when these events are caused by other factions. This also removes excessive friction and treaty penalties when another faction alters rainfall patterns during terraforming.
6. Fix a bug that occurs after the player does an artillery attack on unoccupied tile and then selects another unit to view combat odds and cancels the attack. After this veh_attack_flags will not get properly cleared and the next bombardment on an unoccupied tile always results in the destruction of the bombarding unit.
7. Disable legacy upkeep code in the game engine that might cause AI formers to be reassigned to nearby bases that are owned by other factions.
8. Patch the game engine to use significantly less CPU time when idle by using a method similar to [smac-cpu-fix](https://github.com/vinceho/smac-cpu-fix/). Normally the game uses 100% of CPU time which can be be a problem on laptop devices.
9. When capturing a base, Recycling Tanks and Recreation Commons are not always destroyed unlike previously. They are sometimes randomly destroyed like other facilities (facility_capture_fix).
10. Patch any faction with negative research rating to start accumulating labs on the first turn. In vanilla rules each negative point results in an additional 5 turn delay before the faction starts accumulating labs, for example Believers had a 10 turn delay. Players on two lowest difficulty levels will also start accumulating labs immediately instead of 5 turns later (early_research_start).
11. Fix faction graphics bug that appears when Alpha Centauri.ini has a different set of faction filenames than the loaded scenario file. The patch will make sure the correct graphics set is loaded when opening the scenario. This bug only happened with scenario files, while regular save games were unaffected.
12. Patch the game to use the Command Center maintenance cost that is set in alphax.txt. Normally the game would ignore this value and make the cost dependent on faction's best reactor value which is inconsistent with the other settings for facilities in alphax.txt.
13. Sometimes the base window population row would display superdrones even though they would be suppressed by psych-related effects. The patch removes superdrone icons from the main population row and uses regular worker/drone/talent icons if applicable, so any drones should not be displayed there if they are suppressed by psych effects. To check the superdrone status, open the psych sub window.
14. Fix visual bug where population icons in base window would randomly switch their type when clicking on them.
15. Patch AIs to initiate much less diplomacy dialogs when the player captures their bases. Previously this happened at least once for every turn the AI loses any bases and would repeat the same dialog every time if the player didn't agree to the peace terms. The patch makes the initiation of dialog more dependent on random chance unless the AI would finally accept surrender terms.
16. Patch genetic warfare probe team action to cause much less damage for any units defending the base. In vanilla game mechanics even one attack instantly inflicted almost 80% damage. In the patched version population loss mechanic is unaffected, but even multiple attacks should do substantially less damage for defender units.
17. Patch terrain drawing engine to render more detailed tiles when zooming out from the default level instead of the blocky, less detailed versions. Main menu setup screen will update the planet preview on all resolutions. Interlude and credits backgrounds are scaled on all resolutions. Base window support tab will also render a zoomable terrain view that can be adjusted with the mousewheel (render_high_detail).
18. Modify multiplayer setup screen to use average values for each of the random map generator settings, instead of the highest possible like previously.
19. Fix potential crash when a game is loaded after using Edit Map > Generate/Remove Fungus > No Fungus.
20. Fix foreign base names being visible in unexplored tiles when issuing move to or patrol orders to the tiles.
21. Fix diplomacy dialog to show the missing response messages (GAVEENERGY) when gifting energy credits to another faction.
22. Fix issue that caused sea-based probe teams to be returned to landlocked bases. Probes are now returned to the closest base as determined by the actual pathfinding distance.
23. Fix issue with randomized faction agendas where they might be given agendas that are their opposition social models making the choice unusable. Future social models can be also selected but much less often. Additionally randomized leader personalities always selects at least one AI priority.
24. Fix bug that prevents the turn from advancing after force-ending the turn while any player-owned needlejet in flight has moves left.
25. Game will now allow reselecting units that have already skipped their turns if they have full movement points available (activate_skipped_units).
26. Whenever additional units are added in the editor mode, these are set as independent units requiring no support by default (editor_free_units).
27. Patch Stockpile Energy when it enabled double production if the base produces one item and then switches to Stockpile Energy on the same turn gaining additional credits.
28. Patch Energy Market Crash event to reduce energy reserves only by 1/2 instead of 3/4. Optionally the event can also be disabled entirely (event_market_crash).
29. Bases that have sufficient drone control facilities before the growth phase can grow without triggering possible drone riots on the same turn (delay_drone_riots).
30. Remove drone revolt event due to issues with the original code. Sometimes this caused rioting player-owned bases to join other factions while this did not happen on AI factions.
31. Patch captured base extra drone effect to last a variable time from 20 to 50 turns depending on the captured base size. The AI will also rename captured bases only after they are fully assimilated.
32. When a base is captured that was previously owned by active third faction and the time to assimilate the base was more than zero, the previous owner is preserved after capture.
33. Fix diplomacy dialog appearing multiple times when both human and alien factions are involved in a base capture by removing the event that spawns additional colony pods.
34. Fix missing defender bonus mentioned in the manual "Units in a headquarters base automatically gain +1 Morale when defending".
35. Fix multiple issues in unit morale calculation, see more details in "Improved combat mechanics" section.
36. Fix issue where TECHSHARE faction ability always skips the checks for infiltration conditions while smac_only mode is activated. Spying by probe team, pact, governor or Empath Guild is required.
37. Fix issue where Accelerated Start option may sometimes freeze the game when selecting a random secret project for Hive. Patched version will not assign Citizens Defense Force or Command Nexus for a faction that already has those facilities for free, unless all other choices among the first seven projects have been exhausted, also Empath Guild is always skipped.
38. Disable legacy game startup code that spawned additional colony pods for factions if the difficulty level matched pre-defined rules. The same starting units can now be selected from the config file for all difficulty levels (skip_default_balance).
39. Prevent the AI from making unnecessary trades where it sells their techs for maps owned by the player. The patch removes TRADETECH4 / TRADETECH5 dialogue paths making the AI usually demand a credit payment for any techs.
40. Fix some issues where AI declared war based on calculations using incorrect variables. AI is also less likely to declare war at early stages when it has only few bases built.
41. Fix visual issues where the game sometimes did not update the map properly when recentering it offscreen on native resolution mode.
42. Fix issues where the game would crash on resolutions not divisible by 8. If an unsupported resolution is used, the game will attempt to automatically switch to the closest supported size.
43. Fix rendering bug that caused half of the bottom row map tiles to shift to the wrong side of screen when zoomed out.
44. Fix datalinks window not showing the first character for Sea Formers units.
45. Fix Mind Control issue when capturing bases that have non-allied units owned by a third faction inside them. In this case the units can't remain on the base tile and they are removed.
46. Modify Mind Control probe action to only subvert units inside the base and not on adjacent tiles to balance for the relatively cheap cost for this action.
47. Modify Total Thought Control probe action to not set silently "want revenge" or "shall betray" flags which usually made the AI sneak attack. This action can be still used to subvert bases without declaring war but it applies a notable diplomatic penalty between the factions.
48. Base production picker will not show Paradise Garden as buildable if the base already has Punishment Sphere.
49. Fix issue where terrain detail display on the world map showed incorrect mineral output for aquatic factions.
50. Fix monolith energy to not be limited by tile yield restrictions in the early game. This limitation did not apply on monolith nutrients/minerals.
51. Fix base tile energy output being inconsistent when SE Economy value is between 3 and 4.
52. Fix issue with formers sometimes being able to move after completing improvements on the same turn.
53. Fix GSP defense bonus range sometimes not being accurate at three tiles like the manual implies.
54. Fix inconsistent effects with unit repair facilities and Citizens Defense Force when the base tile is defended by an unit owned by third faction. The facility or the secret project providing it must be built by the base owner for it to have an effect.
55. Fix issues where the game did not calculate upgrade costs correctly when the unit count exceeded 255. The costs will be applied correctly even if Security Nexus may show an inaccurate number.
56. Patch the game to also parse "Free Technology" defined for each facility. This causes new bases to have these facilities built for free while it does not exclude facility maintenance costs.
57. Faction config reader also parses social effect priority/opposition values (e.g. ECONOMY) defined for each faction. These may have minor effect on AI social engineering choices while original game always ignored these values.
58. Modify energy inefficiency formula when the faction does not have headquarters active in which case distance is always treated at least 16. When the faction base count is between 32 and 96, this default distance increases linearly from 16 to 32.
59. Enable COMMFREQ faction ability to grant one extra comm frequency at the game start. Previously this ability was skipped by the game.
60. Fix endgame replay feature overflowing unrelated content causing eventual crash after too many base create/capture/kill events happen during the game.
61. Modify the faction setup code to check by filename if multiple similar factions are active. If any duplicates are found, each one will have their faction_id as name suffix.
62. Fix issue where completely hurrying current production at the base sometimes allowed multiple hurry actions during the same turn.
63. Fix issue after the faction capturing the base with Cloudbase Academy has their aircraft speed altered during the turn resulting in some aircraft crashing when they should not.
64. Fix rare issue that caused the base build queue to be saved with incorrect entries resulting in crashes during turn upkeep.
65. Fix issues that prevented KELPWIPE or PLATFORMWIPE events from happening due to incorrect tile checks. These may now happen at similar probability as other events for suitable bases.
66. Fix NEWRESOURCE event sometimes appearing on a tile that already has bonus resources.
67. Modify config reader to display an error if None/Disable are used as chassis/weapon/armor identifiers for predefined units on alphax.txt.


Scient's patch
==============
Thinker includes the binary patches from both [Scient's patch v2.0 and v2.1](https://github.com/DrazharLn/scient-unofficial-smacx-patch).
The differences between these versions include only changes to the txt files to prevent the game from crashing when opening certain datalinks entries.

Thinker also includes the following non-engine fixes from Scient's patch for specific user interface issues. These labels listed below are redirected from other script files to modmenu.txt based on `script_label` config options.
Installing the other modified files provided by Scient's patch (such as wav files) is purely optional, and Thinker does not include those files by default since it would require overwriting files in the base game.

1. [BUG] Needlejet "DATA" edit window via scenario editor wouldn't render properly making it so you couldn't edit the stats. (EDITVEH2A)
2. [BUG] When attempting to build a sea base inside another faction's territorial waters, you were supposed to receive warning messages that were mislabeled. (TERRTREATY2, TERRTRUCE2)
3. [BUG] Added a new entry that was missing when you attempted to use "B" or "b" shortcuts with a non-combat unit that didn't have a "Colony Pod". (NEEDCOLONISTS)
4. [BUG] Added new entries that were missing in conjunction with an engine fix for when "retool strictness" in alpha/x.txt is set to "never free". (RETOOLPENALTY3, RETOOLPENALTY3A)
5. [BUG][SMAX] When attempting to terraform an ocean square other than shelf, aquatic factions (Nautilus Pirates) were supposed to receive a warning message that was mislabeled. (NEEDADVECOENG)

The following binary fixes from Scient's patch are automatically applied at game startup.

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
25. [BUG][SMAX] Prevent the Caretakers from being given the possibility of building secret project "Ascent to Transcendence" which goes against their philosphy and would cause them to declare war on themselves if built. Problem with logic, code exists which should of prevented it from displaying but didn't.
26. [BUG] Allow tiles with more than one probe ability to evict probes (more than one unit, probe has to be top most) ; when evicting probe, only send top unit to home base -> prevents non-probe units from being booted as well. -> using same code used to return probes to base after successful mission
27. [BUG][SMAX] Fixed bug when AI successfully completes probe action of freeing a captured faction leader (FREEWILLY/FREEWHO) where it would reset a non-captured faction. The problem was that AI would always try to free the very first faction (usually PC) regardless of whether they were eliminated or not. Fix now obtains all potential faction leaders to free and randomly picks one.
28. [BUG][SMAX] Add references to use a new file (movlistx.txt) for SMAX specific info after secret project movies rather than one shared between SMAC/SMAX (movlist.txt).
29. [MOD] Add "Nessus Canyon" landmark placement on random map generation.
30. [BUG] Fixed "attacking along road" combat bonus in alpha/x.txt. Set to 0 by default.
31. [BUG/MOD] Fixed NEWRESOURCE and PETERSOUT events to display the correct corrosponding image to match resource type.
32. [MOD] Add ability to set individual datalink entries for armor and reactors (ARMORDESC#/REACTORDESC#) in help/x.txt.
33. [MOD] Enable the display of Sea Formers unit (UNITDESC4) in datalinks. Also, hide '*' from showing in name.
34. [BUG] Fixed logic in PB code to prevent ODP/Flechette defenses being called a 2nd time (with a chance for defenses to fail) even after PB had already been shot down. Each faction has one chance to defend against incoming missile based on closest base or unit to PB blast. However, there is secondary check to give the tile owner a chance to defend against PB even if they have no units or bases in this space. This check didn't take into account whether or not PB had already been shot down.
35. [BUG/MOD][SMAX] Fungal Missiles would spawn MW in ocean tiles -> fix by spawning IoD instead and adding in one Sealurk instead of Fungal Tower.
36. [BUG] Fixed a bug that would display NERVEGAS message even when attack on base failed -> checking attacker health
37. [MOD] Add ability to set reactor (1-4) for #UNITS in alpha/x.txt (ex. "Colony Pod,..., 00000000000000000000000000,4" -> gives Colony Pods Singularity Engine). To do so, just add comma after Abil with value of reactor you want for unit. If no value is set, it defaults to "1" like original code. For SMAX only, there are two exceptions for Battle Ogre MK2 and Battle Ogre MK3 where default isn't "1" but "2" and "3" respectively. The Ogre default can still be overridden if you set a different reactor value in alpha/x.txt.
38. [BUG] Allowing colony pods to be added to bases with fungus on tile ; allowing sea/land pods to be added to existing sea/land bases ignoring building restriction
39. [BUG] #DISEMBARKFAIL wasn't being displayed due to incorrect logic check ; when you try to move a transport with no units on board (L) onto land this message should display (doesn't include moving into land base, does include two transports on same tile one without units and other with) -> update: added additonal check so #DISEMBARK code wasn't being particially loaded for non-transport units
40. [BUG] Changing start date for Perihelion event to be 2160 from 2190. This is to be consistent with info about Planet and cycle from readme regarding 80 year cycles (20 years near, 60 years far).
41. [BUG][SMAX] FM would sometimes display #FUNGMOTIZED due to faction id of FM memory location would change and instead would use faction id of MW. Fixed by storing faction id in variable.
42. [CRASH] Under rare circumstances, the game would crash when an AI faction with no bases attempted to upgrade a unit.
43. [BUG] Abandoning a base after building a colony pod no longer skips the base production of the next base. This was caused by the upkeep function using incorrect base values after the abandoned base was destroyed. Fixed so acts like #STARVE.
44. [BUG] Movement to and from pact bases should be identical to your own bases. Since a non-amphibious unit can move from a transport into your own sea base, it should be able to move from a transport into a pact mate's base.
45. [BUG][SMAX] Interludes 6 and 7 would display incorrect string variables specific to Caretakers or Usurpers for non-Alien factions. They now show correct values for human factions as well as Aliens.
46. [BUG] Interlude 6 would sometimes be triggered by native life forms causing issues with variables and not making sense, this interlude (and it's follow up #7) are designed for another specific faction. Add check to skip if faction killing unit is alien.
47. [BUG] Fixed display when scenario editor is enabled for changing former owner inside a base (#CHANGEFORMER) since menu didn't load correctly.
48. [BUG] Fixed the parsing of the "Retool strictness" value in alpha/x.txt so "Never Free" works correctly. This would only apply if you wanted to give a retooling penalty when switching to "Secret Projects".
49. [BUG][MOD] Factions with the FREEPROTO flag (Spartans) will gain free retooling in their bases as long as the production switch is within the same category (unit to unit, base facility to base facility) and they've discovered the necessary tech ("Advanced Subatomic Theory"). This is to resolve the issue with FREEPROTO factions never being able to gain the undocumented retooling ability of "Skunkworks" when it is fairly clear that they should.
50. [EXPLOIT] Using the right click "Save current list to template" and "Load template into list" features of base queue can be used to bypass retooling completely. Fixed so these queue template features only save and load the actual queue and not affect the item currently in production.
51. [BUG] When drilling an aquifer, there isn't a check whether a river already exists in the initial square. Now, it checks the the initial square as well as the eight square around it.
52. [BUG] Fixed an issue where diplomacy dialog could be incorrectly displayed due to faction id value being set incorrectly. This is best exhibited where Progenitors switch into "Human" dialog syntax.


Contributions
=============
Source code provided by following people has been incorporated into Thinker Mod in various parts of the project.

* Brendan Casey for [OpenSMACX](https://github.com/b-casey/OpenSMACX) related insights into the game engine and Scient's patch.
* Tim Nevolin's [Will To Power Mod](https://github.com/tnevolin/thinker-doer) for unit healing and reactor power patch and various other game engine config options.
* PlotinusRedux's [PRACX Patch](https://github.com/DrazharLn/pracx) for additional graphics rendering code.
* DrazharLn's [SMAC in SMACX Mod](https://github.com/DrazharLn/smac-in-smax) for modified txt files.
* Pianoslum for German language translation of alphax.txt.
* Markus Hartung for additional cross-platform build scripts for MinGW.

