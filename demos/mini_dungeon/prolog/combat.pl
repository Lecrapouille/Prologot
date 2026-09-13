% =============================================================================
% combat.pl — Who is ALLOWED to attack or cast.
% Consult after dungeon.pl and rules.pl (uses armed/1, visible/2 from there).
%
% Prolog never computes HP. Godot applies damage after a "yes".
% enemy/2 is asserted when each monster is spawned (monster → player).
% =============================================================================

:- dynamic enemy/2.

% Melee: alive, enemy, close enough, visible, AND armed.
% An unarmed goblin therefore cannot hit (it flees / hunts a weapon).
can_attack(Attacker, Target) :-
    alive(Attacker),
    alive(Target),
    enemy(Attacker, Target),
    near(Attacker, Target),
    visible(Attacker, Target),
    armed(Attacker).

% Spell: no need for near/2 or armed/1 — visible/2 is enough.
% Godot fires a fireball (projectile.gd, style "fireball").
can_cast(Wizard, Target) :-
    wizard(Wizard),
    alive(Wizard),
    alive(Target),
    enemy(Wizard, Target),
    visible(Wizard, Target).
