# AGENTS.md — Instructions for AI models

This repository is a Godot 4.7 chess client with a pure C++ rules engine. Read this file before making changes. Detailed references live in `docs/`.

## Invariants (never break these)

1. **`core/domain/` is pure C++17.** It must never include `godot_cpp/*` or any Godot header. It compiles headless so it can be unit-tested and linked by the future `server/`.
2. **All chess rules live in C++.** GDScript must not implement or duplicate rules (move generation, check detection, FEN, turn order). GDScript only presents state and forwards input.
3. **Signals flow up, method calls flow down.** Children (`square.gd`) emit signals; parents (`board.gd`, `main.gd`) react and call methods downward. Do not let children reach into siblings or parents.
4. **Only `board.gd` and autoloads talk to the `ChessGame` facade.** Other scripts go through `board.gd`.
5. **The interop boundary is `client/src/interop/`.** All `Variant` ↔ domain conversions happen there. `chess_game.cpp` composes domain calls; it does not reimplement logic.
6. **No comments in code** (C++ or GDScript), unless explicitly requested. Code style is terse.

## File map

| Change this | Goes here |
|---|---|
| Move generation, check/checkmate/stalemate, castling, en passant | `core/domain/rules.*` |
| Turn, clocks, castling rights, undo history | `core/domain/game_state.*` |
| FEN parse/serialize, `START_FEN` | `core/domain/fen.*` + `game_state.*` |
| Piece/square/move/result value types | `core/domain/types.hpp` |
| 8×8 storage | `core/domain/board.*` |
| Domain tests | `core/tests/test_rules.cpp` |
| GDScript-visible API, selection state machine | `client/src/chess_game.{h,cpp}` |
| Variant ↔ domain conversion | `client/src/interop/convert.{hpp,cpp}` |
| Class registration, entry symbol | `client/src/register_types.cpp` |
| Click handling, highlights, piece spawning | `client/scenes/board/board.gd` |
| Square input / colors | `client/scenes/board/square.gd`, `square.tscn` |
| Piece visuals (placeholder polygons) | `client/scenes/board/piece.gd`, `piece.tscn` |
| HUD status text | `client/scenes/ui/hud.gd` |
| Top-level wiring | `client/scenes/main/main.gd`, `main.tscn` |
| Autoload state stubs | `client/autoload/*.gd` |
| DLL mapping, entry symbol | `client/bin/chess.gdextension` |

## Commands

Run from PowerShell on Windows.

```powershell
# Core tests (run first after any core/ change)
cd core
scons
.\tests\chess_core_tests.exe          # expect: 30 checks, 0 failures

# Build the extension (from client/)
cd client
scons platform=windows target=template_debug arch=x86_64 api_version=4.7
scons platform=windows target=template_release arch=x86_64 api_version=4.7

# Regenerate clangd database at repo root
scons platform=windows target=template_debug arch=x86_64 api_version=4.7 compiledb=yes compiledb_file=../compile_commands.json
```

If `scons` is not on PATH in an agent shell, use:
`& "C:\Users\Alvaro\AppData\Roaming\Python\Python311\Scripts\scons.exe" <args>`

`api_version=4.7` is required — godot-cpp has no `godot-4.7` tag (master @ 507ed9d is used).

GDExtension DLLs hot-reload when the editor window regains focus; no restart needed.

## Contracts (do not change keys without updating GDScript)

- `ChessGame.select_square(coords)` → `{ selected: bool, moved: bool, moves: Array[Vector2i], result: Dictionary }`
- Result dictionary keys: `legal`, `captured`, `en_passant`, `castled`, `promoted`, `check`, `checkmate`, `stalemate`, `fen`, `turn`
- `get_pieces()` → `Array` of `{ square: Vector2i, color: "white"|"black", type: "pawn"|"knight"|"bishop"|"rook"|"queen"|"king" }`
- Coordinates: `Vector2i(file, rank)`, both `0..7`, a1 = `(0, 0)`. On screen, rank is flipped: `position = (Vector2(file, 7 - rank) + Vector2(0.5, 0.5)) * 64`.
- Colors are lowercase strings (`"white"`, `"black"`).

## GDScript gotchas

- `const` values must be compile-time literals. Constructors like `PackedVector2Array(...)` are **not** constant expressions → declare such dictionaries with `var` (see `piece.gd` `SHAPES`).
- `PackedVector2Array(flat, ints, ...)` works in `.tscn` files only, **not** in GDScript. In GDScript use `PackedVector2Array([Vector2(a, b), ...])`.
- C++17 is the standard — no defaulted comparison operators (`= default` for `operator==` needs C++20); write `operator==` / `operator!=` explicitly (see `types.hpp`).
- MSVC needs `/EHsc` (already in both SCons files).

## Known limitations

- Promotion via UI is always **queen** (`select_square` builds a `Move` with default promotion; `GameState::try_move` matches the queen candidate).
- Piece visuals are placeholder `Polygon2D` shapes; sprite art slots exist under `client/assets/sprites/`.
- `server/` is empty; the planned sync format is FEN (`GameState::to_fen` / `load_fen`).
- `client/node_2d.*`, `client/src/test.gd`, `client/src/node_2d.tscn` are leftover editor templates.

## Deeper docs

- `docs/architecture.md` — structure and data flow
- `docs/core-api.md` — domain API
- `docs/facade-api.md` — GDExtension API
- `docs/scenes.md` — scenes and signals
- `docs/contributing.md` — conventions
