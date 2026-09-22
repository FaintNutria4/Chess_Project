# Core API (`core/domain/`)

Pure C++17 chess domain. No Godot includes — this library is shared between the client GDExtension and the future server.

Namespace: `chess` (rules functions in `chess::rules`).

## `types.hpp`

Value types. All comparisons are explicit (C++17; no `= default` comparisons).

```cpp
enum class Color : uint8_t { White = 0, Black = 1 };
enum class PieceType : uint8_t { Pawn, Knight, Bishop, Rook, Queen, King };

Color opposite(Color color);
```

### `Piece`

```cpp
struct Piece {
    Color color = Color::White;
    PieceType type = PieceType::Pawn;
};
```

Equality: both fields. A missing piece is represented by `std::optional` absence, not by a sentinel `Piece`.

### `Square`

```cpp
struct Square {
    int8_t file = 0;   // 0..7, a=0
    int8_t rank = 0;   // 0..7, rank 1=0

    bool valid() const;            // file and rank in [0, 8)
    int index() const;             // rank * 8 + file
    static Square from_index(int index);
};
```

**Coordinate convention:** a1 = `{0, 0}`, h8 = `{7, 7}`. Mirrored as `Vector2i(file, rank)` in GDScript.

### `Move`

```cpp
struct Move {
    Square from;
    Square to;
    bool is_promotion = false;
    PieceType promotion = PieceType::Queen;
};
```

### `MoveResult`

Returned by `GameState::try_move`. Flags describe what just happened:

| Field | Meaning |
|---|---|
| `legal` | Move was applied; all other fields meaningful only if true |
| `captured` | A piece was removed (including en passant) |
| `en_passant` | Capture was an en passant capture |
| `castled` | King moved two files (castle completed) |
| `promoted` | Applied move was a promotion |
| `check` | Side to move (after the move) is in check |
| `checkmate` | Check and no legal moves |
| `stalemate` | No check and no legal moves |

## `board.hpp`

```cpp
class Board {
public:
    void clear();
    void set_piece(const Square &square, const Piece &piece);
    void remove_piece(const Square &square);
    std::optional<Piece> at(const Square &square) const;
private:
    std::array<std::optional<Piece>, 64> cells_{};
};
```

`at()` on an out-of-range square returns `std::nullopt` (via bounds-checked storage layout: index is `rank * 8 + file` through `Square::index()` semantics — callers should keep squares valid).

## `rules.hpp` — `chess::rules`

```cpp
std::vector<Move> pseudo_moves_from(const GameState &state, const Square &from);
std::vector<Move> legal_moves(const GameState &state, const Square &from);
std::vector<Move> all_legal_moves(const GameState &state, Color color);
bool has_legal_move(const GameState &state, Color color);
bool square_attacked(const Board &board, const Square &target, Color by);
std::optional<Square> find_king(const Board &board, Color color);
void apply_move(Board &board, const Move &move,
                const std::optional<Square> &en_passant,
                const Piece &mover);
```

- `pseudo_moves_from` — geometric moves without pin/check filtering.
- `legal_moves` — pseudo moves filtered so the mover's king is not left in check (handles pins, discovered checks, castling through/in check).
- Promotion moves are emitted with `is_promotion = true` and one candidate per promoting piece (queen, rook, bishop, knight).
- `apply_move` mutates the board only (castling rook hop, en passant removal, promotion piece swap); it does not touch turn, rights, or clocks — `GameState::try_move` owns that.

## `game_state.hpp`

### `CastlingRights`

```cpp
struct CastlingRights {
    bool white_king_side = true;
    bool white_queen_side = true;
    bool black_king_side = true;
    bool black_queen_side = true;
};
```

### `StateSnapshot`

Full immutable copy of state used for undo: `board`, `turn`, `rights`, `en_passant`, `halfmove_clock`, `fullmove_number`.

### `GameState`

```cpp
class GameState {
public:
    GameState();                       // resets to START_FEN

    void reset();                      // load START_FEN

    const Board &board() const;
    Color turn() const;
    const CastlingRights &castling_rights() const;
    const std::optional<Square> &en_passant_target() const;

    bool is_in_check(Color color) const;
    std::vector<Move> legal_moves_from(const Square &from) const;
    MoveResult try_move(const Move &move);
    bool undo();                       // false if history empty

    std::string to_fen() const;
    bool load_fen(const std::string &fen);
private:
    std::vector<StateSnapshot> history_;   // stack for undo
};
```

**`try_move` semantics** (important for callers):

1. Rejects invalid/from==to/wrong-turn/no-piece moves → `legal = false`, state unchanged.
2. Matches the request against `legal_moves(from)` by `from`/`to`:
   - Candidate is a promotion: matches if `move.is_promotion && move.promotion == candidate.promotion`, **or** if `!move.is_promotion && candidate.promotion == Queen` (implicit queen promotion).
   - Candidate is not a promotion: matches regardless of the request's promotion flags.
3. On success: pushes a snapshot to history, applies the move, updates en passant target, castling rights (king move, rook corner departures/arrivals), halfmove clock (reset on pawn move or capture), fullmove number, flips turn, then computes `check` / `checkmate` / `stalemate` for the new side to move.

**`undo`:** restores the most recent snapshot (board, turn, rights, en passant, clocks) and pops history. Returns `false` when there is nothing to undo.

## `fen.hpp`

```cpp
inline constexpr const char *START_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
```

`GameState::to_fen()` / `load_fen()` implement the standard 6-field FEN. Round-trip is exact (tested). FEN is the intended network state-sync format.

## Tests (`core/tests/test_rules.cpp`)

Headless suite, no framework — a `CHECK` macro counts checks/failures.

```powershell
cd core
scons
.\tests\chess_core_tests.exe    # 30 checks, 0 failures
```

Covered today: initial 20 legal moves; `e2-e4` FEN + en passant target; illegal long pawn move rejected with state unchanged; en passant capture; kingside castling (rook placement); pinned piece has no moves; fool's-mate checkmate; stalemate; undo after two plies; FEN round-trip.

**Adding tests:** append blocks in `main()` in `test_rules.cpp`, use `CHECK(...)` (wrap brace-initializers in extra parens: `CHECK((sq == Square{1, 2}))` — the macro takes one argument). Re-run `scons` and the executable; update the expected count in docs if you add checks.
