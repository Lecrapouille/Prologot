# Note for Prolog Developers

## Compatibility with SWI-Prolog

Prologot uses SWI-Prolog 8.0+ and supports all standard Prolog features. The underlying Prolog engine is fully compatible with standard Prolog code, so you can use any Prolog code that works with SWI-Prolog.

## Syntax Variation

Prologot uses modern, intuitive method names to make it easier for non-Prologists to get started. If you are an experienced Prolog developer, here is the mapping from Prologot's names to traditional Prolog terms.

| Prologot Name         | Traditional Prolog Name                 | Description                                    |
|-----------------------|-----------------------------------------|------------------------------------------------|
| `consult_file()`         | `consult/1`                             | Load a Prolog file                             |
| `consult_string()`         | `consult_string/1` or `assertz/1`       | Load Prolog code from a string                 |
| `add_fact()`          | `assert/1` or `assertz/1`               | Add a fact to the knowledge base               |
| `retract_fact()`       | `retract/1`                             | Remove a fact from the knowledge base          |
| `retract_all()`       | `retractall/1`                          | Remove all facts matching a pattern            |
| `solve()` / `solve_all()` / `solve_one()` | term construction + `call/1` | High-level API: functor + Godot values, no source parsing |
| `query_text()` / `query_text_all()` / `query_text_one()` | `call/1` + `findall/3` on a source string | Low-level API: parse Prolog text |

- Modern names are more intuitive for developers coming from other languages (Python, JavaScript, etc.).
- Names follow conventions from the Godot/GDScript ecosystem.

## Prolog Syntax Notes

- **Variable naming**: In Prolog, variables must start with uppercase or underscore. Lowercase names are atoms (constants). This is important when naming game entities - use lowercase for character names (atoms) but uppercase for query variables.
- **No trailing periods in queries**: Prologot automatically ignores trailing periods from query strings, facts, and functors. This makes the API more forgiving.
- **Two query APIs**: use different names on purpose.
  - High-level `solve_all("parent", ["X", "Y"])` builds a Prolog term from Godot values and returns variable bindings: `[{"X": "tom", "Y": "bob"}, ...]`.
  - Low-level `query_text_all("parent(X, Y)")` parses a Prolog source string and returns compound terms: `[{"functor": "parent", "args": ["tom", "bob"]}, ...]`.
- **Why two names?** `solve()` never parses Prolog text: a source string such as `"parent(tom, X)"` is rejected. `query_text()` is the escape hatch for conjunctions, operators, and the editor console. Keeping the names distinct makes the architecture obvious.
- **Knowledge base accumulation**: Multiple calls to `consult_file()` and `consult_string()` accumulate clauses. Use `retract_all()` to remove specific predicates if needed.