% Pathfinding - Graph Traversal
% Find paths between nodes with cycle detection

% Bidirectional connections between nodes
edge(a, b, 1).
edge(b, c, 2).
edge(b, d, 3).
edge(c, e, 1).
edge(d, e, 2).
edge(e, f, 1).

connected(X, Y, Cost) :- edge(X, Y, Cost).
connected(X, Y, Cost) :- edge(Y, X, Cost).

% Find a path with cycle detection
path(Start, End, Path, Cost) :-
	path_helper(Start, End, [Start], ReversePath, Cost),
	reverse(ReversePath, Path).

% Base case: reached destination
path_helper(End, End, Visited, Visited, 0).

% Recursive case: move to next node
path_helper(Current, End, Visited, Path, TotalCost) :-
	connected(Current, Next, Cost),
	\+ member(Next, Visited),  % Avoid cycles
	path_helper(Next, End, [Next|Visited], Path, RestCost),
	TotalCost is Cost + RestCost.
