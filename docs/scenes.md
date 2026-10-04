# Scenes and Signal Contracts

GDScript layer of the client: composition, input, rendering, HUD. No chess rules live here — everything rule-related arrives through the `ChessGame` facade (see `facade-api.md`).

## Scene tree at runtime

```
Main (Node2D, main.gd)                    scenes/main/main.tscn  ← main scene
├── Board (Node2D, board.gd)              scenes/board/board.tscn
│   ├── Squares (Node2D)                  64 runtime-instanced squares
│   │   └── Square (Area2D, square.gd) ×64 scenes/board/square.tscn
│   │       ├── Base (Polygon2D)              board color
│   │       ├── Highlight (Polygon2D)         green overlay, hidden by default
│   │       └── CollisionShape2D              64×64 RectangleShape2D
│   └── Pieces (Node2D)                   runtime piece instances
│       └── Piece (Node2D, piece.gd) ×N    scenes/board/piece.tscn
│           └── Visual (Polygon2D)         placeholder silhouette
│   └── Promotion (CanvasLayer, promotion.gd)  scenes/board/promotion.tscn (hidden by default)
│       └── Panel (PanelContainer)
│           └── Pieces (HBoxContainer) → Queen/Rook/Bishop/Knight (Button)
└── UI (CanvasLayer, hud.gd)              scenes/ui/hud.tscn
    └── StatusLabel (Label)

Autoloads (project.godot):
├── GameSession (game_session.gd)         player_color, networked
└── Network (network.gd)                  move_received etc. (stub)
```

Main scene is set in `project.godot`: `run/main_scene="res://scenes/main/main.tscn"`.

## Wiring (main.gd)

```gdscript
func _ready() -> void:
    $Board.move_played.connect(_on_move_played)

func _on_move_played(result: Dictionary) -> void:
    # text starts empty; only result["checkmate"], result["stalemate"], result["check"]
    # produce output ("Checkmate! X wins" uses result["turn"] as the side that lost)
    # a plain move clears the previous message
```

The HUD never shows whose turn it is — the label exists for check / checkmate / stalemate only.

## Signals

| Signal | Emitted by | Payload | Consumed by |
|---|---|---|---|
| `clicked(square)` | `square.gd` (left mouse press on Area2D) | the `Area2D` node (has `coords`) | `board.gd` → `_on_square_clicked` |
| `piece_chosen(piece_name)` | `promotion.gd` (picker button press) | `"queen"` / `"rook"` / `"bishop"` / `"knight"` | `board.gd` → `_on_promotion_chosen` |
| `move_played(result)` | `board.gd` after a successful `try_move` | result Dictionary (facade contract) | `main.gd` → HUD status |
| `connected` / `disconnected` | `network.gd` (autoload, stub) | — | future lobby UI |
| `move_received(from, to)` | `network.gd` (autoload, stub) | `Vector2i, Vector2i` | future online board sync |

Direction rule: **children emit, parents subscribe**. Parents call child methods directly (no reverse signals).

## Script contracts

### `board.gd` (controller/presenter) — owns the facade

```gdscript
var chess: ChessGame          # created in _ready()

func square_position(coords: Vector2i) -> Vector2
    # (Vector2(file, 7 - rank) + Vector2(0.5, 0.5)) * 64.0
    # a1 → bottom-left cell, board occupies 512×512 px, origin top-left of board
```

- `_ready`: `ChessGame.new()` → connect `$Promotion.piece_chosen` → `_build_squares()` → `_sync_pieces()`.
- `_build_squares`: instances 64 squares, sets `coords`, base color, connects `clicked`.
- `_on_square_clicked(square)`:
  - a pending promotion ignores clicks on its own `from`/`to` squares; any other square click cancels it first;
  - `chess.select_square(square.coords)` → always clears previous highlights;
  - `move` returned → if `move["promotion"]` store it in `pending_move` and open the picker, else `_submit_move(move, "")`;
  - `selected` → highlight each entry's `to` in `moves`.
- `_submit_move(move, piece_name)`: **one** `chess.try_move(from, to, piece_name)` call → `_cancel_pending()` → on `legal` `_sync_pieces()` + emit `move_played(result)`. Nothing is applied before this call, so cancelling costs nothing.
- `_on_promotion_chosen(piece_name)`: forwards `pending_move` to `_submit_move`.
- `_cancel_pending()`: clears `pending_move`, closes the picker, clears highlights, `chess.deselect()`. Triggered by Escape, right-click (`_unhandled_input`), or clicking another square.
- `_sync_pieces`: frees all piece nodes, respawns one per `chess.get_pieces()` entry (full respawn — simple, correct; upgrade path is incremental updates without facade changes).
- Colors: `LIGHT_COLOR` / `DARK_COLOR`; light when `(file + rank) % 2 == 0` (so a1 renders light in this project).

Public surface for other scripts: signal `move_played`. Nobody else constructs `ChessGame`.

### `promotion.gd` (picker)

```gdscript
signal piece_chosen(piece_name: String)   # "queen" / "rook" / "bishop" / "knight"

func open(color: String, screen_pos: Vector2)   # icon set per color, shown centered on screen_pos
func close() -> void
```

Non-modal `CanvasLayer` (layer 10) so board clicks still reach `board.gd` while it is open; its `PanelContainer` swallows mouse input (including right-clicks) so only clicks on the four buttons act. Icons are 64 px rescaled copies of `piece.gd`'s `TEXTURES`. Presentation only — no `chess` access, no rules.

### `square.gd` (input)

```gdscript
signal clicked(square)                 # emits self
var coords: Vector2i = Vector2i.ZERO   # set by board.gd before add_child

func set_base_color(color: Color)      # $Base.color
func set_highlight(enabled: bool)      # $Highlight.visible
```

Handles `InputEventMouseButton` (left, pressed) via `input_event`. Knows nothing about pieces or rules.

### `piece.gd` (view)

```gdscript
var SHAPES: Dictionary    # piece type → PackedVector2Array silhouette (var, not const!)
var piece_color: String
var piece_type: String

func setup(color_name: String, type_name: String)
    # sets fields, $Visual.polygon from SHAPES, fill color (light / dark)
```

**`SHAPES` must stay `var`:** `const` requires compile-time literals, and `PackedVector2Array([...])` is a constructor call — GDScript rejects it in `const` context (`assigned value for constant isn't a constant expression`). Also note the flat-int form `PackedVector2Array(a, b, ...)` is `.tscn`-only; scripts need `Vector2` elements.

Placeholder art: swap `$Visual.polygon` for a `Sprite2D` later; sprites go in `client/assets/sprites/`.

### `hud.gd`

```gdscript
func set_status(text: String)   # $StatusLabel.text
```

### Autoloads

- `GameSession`: `player_color: String = "white"`, `networked := false`. Future: which side the human plays.
- `Network`: `active`, `connect_to_server(address, port)` (no-op stub), `send_move(from, to)` (no-op), signals `connected`, `disconnected`, `move_received(from, to)`. Filled in when `server/` exists; sync format will be FEN-based.

## Layout constants

| Constant | Value | Where |
|---|---|---|
| Square size | 64 px | `board.gd` `SQUARE_SIZE`, `square.tscn` shape |
| Board extent | 512×512 | 8 × 64 |
| Window stretch | `canvas_items` / `expand` | `project.godot` |
| Piece silhouette extent | roughly ±20 px | `piece.gd` `SHAPES` |

## Adding UI without breaking contracts

1. New top-level UI → instance it in `main.tscn`, wire it in `main.gd`.
2. Board-related UI → listen to `move_played`; do not call `chess` directly.
3. Online move intake (future) → `Network.move_received` should feed `board.gd` (call facade + `_sync_pieces`), keeping rule evaluation in C++.
