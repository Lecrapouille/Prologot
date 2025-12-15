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
| `query_all()`  (with custom syntax)   | `findall/3` | Returns all solutions. Arguments differ from traditional Prolog. See explanations below. |

- Modern names are more intuitive for developers coming from other languages (Python, JavaScript, etc.).
- Names follow conventions from the Godot/GDScript ecosystem.

## Prolog Syntax Notes

- **Variable naming**: In Prolog, variables must start with uppercase or underscore. Lowercase names are atoms (constants). This is important when naming game entities - use lowercase for character names (atoms) but uppercase for query variables.
- **No trailing periods in queries**: Prologot automatically ignores trailing periods from query strings, facts, and functors. This makes the API more forgiving.
- **Variable extraction**: `query_all` returns an array of dictionaries, which is a convenient way to maps variable names to their values. There are two query syntaxes with different return formats:
  - Legacy format `query_all("parent(X, Y)")` returns compound terms: `[{"functor": "parent", "args": ["tom", "bob"]}, ...]`.
  - Extraction format `query_all("parent", ["X", "Y"])` returns variable bindings: `[{"X": "tom", "Y": "bob"}, ...]`.
- **Why two syntaxes?** The legacy format (`query_all("parent(X, Y)")`) passes the entire query as a string, so Prologot cannot know which variables you want to extract. It would require parsing the string to identify variable names, which is complex, fragile, and error-prone (handling nested parentheses, spaces, complex terms, etc.). The new format (`query_all("parent", ["X", "Y"])`) explicitly provides the variable names, making extraction straightforward and reliable. This design choice prioritizes correctness and maintainability over convenience.
- **Knowledge base accumulation**: Multiple calls to `consult_file()` and `consult_string()` accumulate clauses. Use `retract_all()` to remove specific predicates if needed.