% Fixture used by étape 0 consult_file tests.
animal(dog).
animal(cat).
animal(bird).
member_of(X, [dog, cat, bird]) :- animal(X).
