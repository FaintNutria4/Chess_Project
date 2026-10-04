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
| `try_move(from: Vector2i, to: Vector2i, promotion: String)` | `Dictionary` | **The only method that applies a move** (see matching rules below). Empty `promotion` = ordinary move (or implicit queen if the destination requires promotion); `"rook"`/`"bishop"`/`"knight"` are explicit, anything else is queen. Clears selection. Returns the result dictionary, or `legal: false` for illegal moves. |
| `legal_moves_for(square: Vector2i)` | `Array[Dictionary]` | Legal moves for the piece on `square`; each entry `{ from: Vector2i, to: Vector2i, promotion: bool }` (empty `Array` if invalid/empty square). Promotion destinations appear once with `promotion: true`. |
| `get_pieces()` | `Array[Dictionary]` | Every occupied square, rank-major order. Each entry: `{ square: Vector2i, color: String, type: String }`. |

### Selection state machine (used by `board.gd`)

```gdscript
var response: Dictionary = chess.select_square(coords)
```

`select_square` owns selection internally (`selected` square + `has_selected` flag) and **never applies moves**:

- **Click own piece** → selects it, returns its legal moves to highlight.
- **Click a destination while selected** → clears selection and returns the chosen `move` dict; the caller applies it with `try_move` (opening the promotion picker first when `move["promotion"]` is `true`).
- **Any other click** → clears selection, re-selecting if the clicked square holds a piece of the side to move.

**Response contract** (all keys always present):

```gdscript
{
    "selected": bool,             # a piece is now selected
    "moves": Array[Dictionary],   # legal moves when selected: { from, to, promotion }
    "move": Dictionary            # { from, to, promotion } when the click completed a move choice, else {}
}
```

`deselect()` clears the selection and any highlighted moves — used by `board.gd` to cancel a pending promotion.

### Property

| Property | Type | Notes |
|---|---|---|
| `player_name` | `String` | Setter/getter bound; exposed via `ADD_PROPERTY`. Used by `hello()`. |

### Legacy

| Method | Returns | Description |
|---|---|---|
| `hello()` | `String` | `"Hello from C++! Player: " + player_name` — connectivity smoke test. |

## Result dictionary (from `try_move`)

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

`main.gd` reads `checkmate`, `stalemate`, `check` to build the HUD string (`turn` only to name the checkmate winner); the label carries no turn indicator.

## Implementation notes

- Selection state (`selected`, `has_selected`) lives in the facade, not in GDScript.
- `select_square` never builds or applies a `Move` for a destination click; it only reports the move dict (`move_to_dict` in `interop/convert.*`). Applying is exclusively `try_move`, so a promotion is resolved in a **single call** carrying `from`, `to` and the chosen piece.
- `GameState::try_move` matches a requested promotion piece against the 4 generated candidates; an empty `promotion` maps to queen (implicit queen fallback for a promotion destination).
- Move lists are deduplicated by destination: the four promotion candidates collapse into one entry with `promotion: true`, so the UI can pop the promotion picker for that destination.
- Illegal moves return `legal: false` and never mutate state.
