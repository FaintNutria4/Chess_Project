# GDExtension Facade API (`ChessGame`)

`ChessGame` is the only class exposed to GDScript. It is a `RefCounted` GDCLASS registered at scene initialization level, so GDScript creates it with `ChessGame.new()`.

## Registration

- Entry symbol: `chess_library_init` (`client/src/register_types.cpp`), declared in `client/bin/chess.gdextension` with `compatibility_minimum = "4.7"`.
- Libraries: `bin/libchess.windows.template_debug.x86_64.dll` / `...template_release...dll`.
- `GDREGISTER_CLASS(ChessGame)` runs at `MODULE_INITIALIZATION_LEVEL_SCENE`.

Rebuild from `client/`:

```powershell
scons platform=windows target=template_debug arch=x86_64 api_version=4.7
scons platform=windows target=template_release arch=x86_64 api_version=4.7
```

DLLs hot-reload when the editor regains focus.

## Coordinates and strings

| Concept | GDScript | C++ domain |
|---|---|---|
| Square | `Vector2i(file, rank)`, `0..7`, a1 = `(0, 0)` | `chess::Square{file, rank}` |
| Color | `"white"` / `"black"` | `chess::Color` |
| Piece type | `"pawn"`, `"knight"`, `"bishop"`, `"rook"`, `"queen"`, `"king"` | `chess::PieceType` |

Conversions live in `client/src/interop/convert.*`. `piece_type_from_name` maps `"rook"`/`"bishop"`/`"knight"` explicitly; **anything else maps to queen** (used for default/empty promotion).

## Methods

### Game control

| Method | Returns | Description |
|---|---|---|
| `reset()` | `void` | Reloads `START_FEN`, clears selection. |
| `get_fen()` | `String` | Current position as FEN. |
| `load_fen(fen: String)` | `bool` | Parses FEN; on success clears selection, on failure leaves state unchanged. |
| `get_turn()` | `String` | `"white"` or `"black"` — side to move **after** the last applied move. |
| `is_in_check()` | `bool` | Whether the side to move is in check. |

### Moves

| Method | Returns | Description |
|---|---|---|
| `try_move(from: Vector2i, to: Vector2i, promotion: String)` | `Dictionary` | Low-level move application. Delegates to `GameState::try_move` (see matching rules below). Promotion strings other than rook/bishop/knight are treated as queen. |
| `legal_moves_for(square: Vector2i)` | `Array[Vector2i]` | Destination squares for the piece on `square` (empty `Array` if invalid/empty square). |
| `get_pieces()` | `Array[Dictionary]` | Every occupied square, rank-major order. Each entry: `{ square: Vector2i, color: String, type: String }`. |

### Selection state machine (used by `board.gd`)

```gdscript
var response: Dictionary = chess.select_square(coords)
```

`select_square` owns selection internally (`selected` square + `has_selected` flag):

- **Click own piece** → selects it, returns destinations to highlight.
- **Click another square while selected**:
  - destination reachable → applies the move (implicit **queen** promotion — the built `Move` uses defaults), returns `moved: true` with the result;
  - otherwise → falls through: clears selection, and re-selects if the clicked square holds a piece of the side to move.

**Response contract** (all keys always present):

```gdscript
{
    "selected": bool,          # a piece is now selected
    "moved": bool,             # a move was applied this call
    "moves": Array[Vector2i],  # legal destinations (when selected)
    "result": Dictionary       # move result (when moved), else {}
}
```

### Property

| Property | Type | Notes |
|---|---|---|
| `player_name` | `String` | Setter/getter bound; exposed via `ADD_PROPERTY`. Used by `hello()`. |

### Legacy

| Method | Returns | Description |
|---|---|---|
| `hello()` | `String` | `"Hello from C++! Player: " + player_name` — connectivity smoke test. |

## Result dictionary (from `try_move` / `select_square` → `result`)

Built by `chess::interop::result_to_dict`:

| Key | Type | Meaning |
|---|---|---|
| `legal` | `bool` | Move applied |
| `captured` | `bool` | Piece removed (incl. en passant) |
| `en_passant` | `bool` | Capture was en passant |
| `castled` | `bool` | Castle completed |
| `promoted` | `bool` | Applied move was a promotion |
| `check` | `bool` | New side to move is in check |
| `checkmate` | `bool` | Checkmate |
| `stalemate` | `bool` | Stalemate |
| `fen` | `String` | Position after the move |
| `turn` | `String` | Side to move next (`"white"` / `"black"`) |

`main.gd` reads `turn`, `checkmate`, `stalemate`, `check` to build the HUD string.

## Implementation notes

- Selection state (`selected`, `has_selected`) lives in the facade, not in GDScript.
- `select_square` builds plain `Move{from, to}` values; via `GameState::try_move` matching, a promotion to the 8th rank therefore always becomes a **queen** promotion (candidate with `promotion == Queen` matches a non-promotion request).
- `try_move` sets `is_promotion = true` and resolves `promotion` through `piece_type_from_name`; non-promotion candidates still match regardless (see `core-api.md`), so the method is safe for ordinary moves too.
- Illegal moves return `legal: false` and never mutate state.
