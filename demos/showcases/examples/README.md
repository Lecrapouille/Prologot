# Prologot Examples

This directory contains Prolog example files used by the interactive demo.

## File Format

Each `.pl` file contains pure Prolog code that can be loaded with `prolog.consult_file()`.

## Examples

- **01_basic_queries.pl** - Simple facts about family relationships
- **02_facts_and_rules.pl** - Rules for grandparents, siblings, and ancestors
- **03_dynamic_assertions.pl** - Dynamic predicate declaration
- **04_complex_queries.pl** - Game combat system with calculations
- **05_pathfinding.pl** - Graph traversal with cycle detection
- **06_ai_behavior.pl** - AI decision-making system

## Configuration

The `examples.json` file contains metadata for each example:

```json
{
  "title": "Display title",
  "file": "res://examples/filename.pl",
  "description": "What to try..."
}
```

## Adding Examples

1. Create a new `.pl` file with Prolog code
2. Add metadata to `examples.json`
3. Add execution function in `prologot-demos.gd` inside the function `_on_execute_button_pressed()`

Keep examples focused and well-commented to showcase specific Prologot features.
