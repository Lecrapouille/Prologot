% =============================================================================
% ai.pl — Monster decisions.
%
% Godot calls best_action(Who, Action, Target) about every 0.25 s.
% The cut (!) commits to the FIRST succeeding clause: the order below IS
% the priority (drink > seek weapon > flee > cast > back off > hit > …).
% =============================================================================

% Potion in inventory and wounded (or player) → heal.
action(Who, drink, Potion) :-
    can_drink(Who, Potion).

% Unarmed goblin that sees a floor weapon → walk to it (Godot move_towards).
action(Goblin, seek_weapon, Item) :-
    goblin(Goblin),
    seek_weapon(Goblin, Item).

% Unarmed + player visible → flee (no unarmed melee).
action(Goblin, flee, Player) :-
    goblin(Goblin),
    unarmed(Goblin),
    player(Player),
    alive(Player),
    visible(Goblin, Player).

% Second flee rule: wounded goblins flee even when armed (first rule was unarmed).
action(Goblin, flee, Player) :-
    goblin(Goblin),
    wounded(Goblin),
    player(Player),
    alive(Player).

% Wizard: can_cast/2 holds as soon as the player is visible → fireball.
action(Wizard, cast, Player) :-
    wizard(Wizard),
    player(Player),
    can_cast(Wizard, Player).

% Too close for melee: the wizard backs off (keep_distance), it does not hit.
action(Wizard, keep_distance, Player) :-
    wizard(Wizard),
    player(Player),
    alive(Player),
    near(Wizard, Player).

% Armed goblin in contact → attack (Godot: goblin_attack).
action(Goblin, attack, Player) :-
    goblin(Goblin),
    player(Player),
    can_attack(Goblin, Player).

% Armed goblin that sees the player but is not near/2 → chase.
action(Goblin, chase, Player) :-
    goblin(Goblin),
    armed(Goblin),
    player(Player),
    alive(Player),
    visible(Goblin, Player),
    \+ near(Goblin, Player).

% Fallback: random patrol (Godot: wander_dir).
action(Who, wander, Who) :-
    monster(Who),
    alive(Who).

% --- Single choice: the cut blocks a lower-priority action -------------------
best_action(Who, drink, Potion) :-
    action(Who, drink, Potion), !.

best_action(Who, seek_weapon, Item) :-
    action(Who, seek_weapon, Item), !.

best_action(Who, flee, Player) :-
    action(Who, flee, Player), !.

best_action(Who, cast, Player) :-
    action(Who, cast, Player), !.

best_action(Who, keep_distance, Player) :-
    action(Who, keep_distance, Player), !.

best_action(Who, attack, Player) :-
    action(Who, attack, Player), !.

best_action(Who, chase, Player) :-
    action(Who, chase, Player), !.

% No cut here: wander is the lowest priority and always succeeds for live monsters.
best_action(Who, wander, Who) :-
    monster(Who).
