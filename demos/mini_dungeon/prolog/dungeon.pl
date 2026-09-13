% =============================================================================
% dungeon.pl — Entity types and spatial relations.
%
% Godot does NOT send (x, y) positions to Prolog. It only asserts dynamic
% facts: "this node is a goblin", "A can see B", etc.
% Godot nodes are Prolog terms (PrologObject).
% =============================================================================

% --- Types (one fact per node registered via PrologGame.register) ------------
:- dynamic player/1.
:- dynamic goblin/1.
:- dynamic wizard/1.
:- dynamic potion/1.
:- dynamic door/1.
:- dynamic chest/1.
:- dynamic stairs_down/1.   % next floor (behind the door on floors 1–3)
:- dynamic stairs_up/1.     % previous floor / surface exit
:- dynamic key/1.           % item type; also a hidden node in $Keep once picked up
:- dynamic sword/1.
:- dynamic bow/1.
:- dynamic arrow/1.         % quiver token — has(Player, Arrow) for can_shoot/1
:- dynamic armor/1.
:- dynamic treasure/1.
:- dynamic on_floor/1.      % pickup still on the ground (not in an inventory)

% --- Vital state (updated when Godot applies damage / healing) ---------------
% alive/1 and dead/1 are mutually exclusive; healthy/1 and wounded/1 likewise.
:- dynamic alive/1.
:- dynamic dead/1.
:- dynamic healthy/1.
:- dynamic wounded/1.

% --- Spatial relations (recomputed each think via sync_spatial) --------------
% near/2    ≈ melee range (~52 px)
% visible/2 ≈ LOS / range (~380 px) — also used to see floor loot
:- dynamic near/2.
:- dynamic visible/2.

% Anything with AI: goblins AND wizards.
monster(X) :- goblin(X).
monster(X) :- wizard(X).

% A "weapon" in the Prolog sense (arrows are not: they only feed can_shoot).
weapon(X) :- sword(X).
weapon(X) :- bow(X).
