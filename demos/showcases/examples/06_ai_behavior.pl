% AI Behavior - Decision Making
% AI state transitions based on conditions

% AI states
state(patrol).
state(chase).
state(attack).
state(flee).

% State transitions
should_chase(PlayerDist) :- PlayerDist < 10.
should_attack(PlayerDist) :- PlayerDist < 3.
should_flee(Health) :- Health < 20.

% Decide action based on health and distance
decide_action(flee, Health, _) :-
	should_flee(Health), !.
decide_action(attack, _, Dist) :-
	should_attack(Dist), !.
decide_action(chase, _, Dist) :-
	should_chase(Dist), !.
decide_action(patrol, _, _).
