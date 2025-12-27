% Complex Queries - Combat System
% Game mechanics with enemies and weapons

% Enemy types: name, hp, attack, defense
enemy(goblin, 10, 5, 2).
enemy(orc, 25, 12, 5).
enemy(dragon, 100, 30, 15).

% Weapons: name, damage
weapon(sword, 10).
weapon(axe, 15).
weapon(bow, 8).

% Calculate damage
damage(Weapon, Enemy, Damage) :-
	weapon(Weapon, WeaponDmg),
	enemy(Enemy, _, _, Defense),
	Damage is WeaponDmg - Defense.

% Check if weapon can one-shot an enemy
one_shot_kill(Weapon, Enemy) :-
	damage(Weapon, Enemy, Dmg),
	enemy(Enemy, HP, _, _),
	Dmg >= HP.
