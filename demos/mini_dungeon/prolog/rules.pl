% =============================================================================
% rules.pl — Inventory, weapons, doors, quest goals.
%
% has/2 is the inventory source of truth. Godot asserts / retracts
% has(Owner, Item) on pickup, drop, and consume.
% Predicates below are DERIVED: Godot queries them, it does not assert them.
% Load order: dungeon.pl → rules.pl → combat.pl → ai.pl (see prolog_game.gd).
% =============================================================================

:- dynamic has/2.
:- dynamic requires/2.   % requires(Door, Key)
:- dynamic opened/1.
:- dynamic at/2.         % at(Player, Stairs) when changing floors

% Armed = owns at least one sword or bow (see weapon/1 in dungeon.pl).
armed(Who) :-
    has(Who, Weapon),
    weapon(Weapon).

% unarmed is defined only for goblins: they flee / hunt a weapon.
unarmed(Who) :-
    goblin(Who),
    \+ armed(Who).

% The player may drink even at full health; a monster only if wounded.
can_drink(Who, Potion) :-
    potion(Potion),
    has(Who, Potion),
    (wounded(Who); player(Who)).

% HUD helper: player has any weapon (sword or bow).
can_fight(Who) :-
    armed(Who).

% Distinct from armed/1: you may own a bow while the sword is still in hand.
holding_sword(Who) :-
    has(Who, Sword),
    sword(Sword).

% Melee = sword in hand. Godot applies damage if near/2 holds.
can_melee(Who) :-
    holding_sword(Who).

% Teaching constraint: you cannot hold the sword AND shoot the bow.
must_drop_sword(Player) :-
    player(Player),
    holding_sword(Player),
    has(Player, Bow),
    bow(Bow).

% Ranged: bow + at least one arrow + no sword in hand (\+ holding_sword).
can_shoot(Who) :-
    has(Who, Bow),
    bow(Bow),
    has(Who, Arrow),
    arrow(Arrow),
    \+ holding_sword(Who).

wearing_armor(Player) :-
    player(Player),
    has(Player, Armor),
    armor(Armor).

% Unarmed goblin that SEES a sword/bow still on the floor → seek_weapon in ai.pl.
seek_weapon(Goblin, Item) :-
    unarmed(Goblin),
    weapon(Item),
    on_floor(Item),
    visible(Goblin, Item).

% Door: matching key via requires/2, and not yet opened/1.
can_open(Player, Door) :-
    player(Player),
    door(Door),
    \+ opened(Door),
    key(Key),
    has(Player, Key),
    requires(Door, Key).

% --- HUD / end-of-run goals (queried by dungeon._refresh_hud and player_go_up) -
completed(Player, find_key) :-
    player(Player),
    key(Key),
    has(Player, Key).

completed(_Player, open_door) :-
    door(Door),
    opened(Door).

completed(Player, find_treasure) :-
    player(Player),
    treasure(Treasure),
    has(Player, Treasure).

% Player stepped on DOWN stairs (asserted in player_go_down before _load_floor).
completed(Player, reach_exit) :-
    player(Player),
    stairs_down(Stairs),
    at(Player, Stairs).

% Victory: treasure in inventory + player standing on floor-1 UP stairs.
completed(Player, reach_surface) :-
    player(Player),
    treasure(Treasure),
    has(Player, Treasure),
    stairs_up(Stairs),
    at(Player, Stairs).
