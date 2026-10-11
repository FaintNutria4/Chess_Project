# Chess Project

A chess game built with **Godot 4.7** (GDScript + 2D scenes) and a **C++ GDExtension** that contains all chess rules. The rules engine lives in a pure C++ `core/` library designed to be shared with the multiplayer `server/` (a Boost.Beast HTTP + WebSocket server, currently an echo placeholder).

## Layout

```
Chess_Project/
├── client/          Godot 4.7 project (scenes, GDScript, GDExtension source)
│   ├── src/         GDExtension: ChessGame facade + interop layer
│   ├── scenes/      board/, main/, ui/ scenes and scripts
│   ├── autoload/    GameSession, Network (stubs)
│   ├── bin/         chess.gdextension + built DLLs
│   └── godot-cpp/   vendored godot-cpp (master, api 4.7)
├── core/            Pure C++17 chess domain (no Godot dependencies)
│   ├── domain/      types, board, rules, game_state, fen
│   └── tests/       headless test suite
├── server/          Boost.Beast HTTP+WebSocket server (echo placeholder; links core/ later)
│   ├── external/    vendored Boost headers (gitignored)
│   └── tests/       connection test (test_connection.ps1)
├── docs/            detailed documentation
├── AGENTS.md        conventions for AI models
└── compile_commands.json   clangd database (repo root)
```

## Prerequisites

- **Godot 4.7** (standard build, Forward+)
- **SCons** 4.x (installed and on PATH; new terminals pick it up automatically)
- **MSVC** build tools (Windows x86_64)

## Quickstart

### 1. Run the core tests (no Godot needed)

```powershell
cd core
scons
.\tests\chess_core_tests.exe
```

Expected: `45 checks, 0 failures`.

### 2. Build the GDExtension

```powershell
cd client
scons platform=windows target=template_debug arch=x86_64 api_version=4.7
scons platform=windows target=template_release arch=x86_64 api_version=4.7
```

This produces `client/bin/libchess.windows.template_debug.x86_64.dll` and the release equivalent.

> **Agent shells:** if `scons` is not found, use the full path:
> `& "C:\Users\Alvaro\AppData\Roaming\Python\Python311\Scripts\scons.exe" <args>`

### 3. Run the game

Open `client/project.godot` in Godot 4.7 and press **F5** (main scene: `scenes/main/main.tscn`).

- Click a piece → legal moves highlight green
- Click a destination → the move plays; the HUD shows turn / check / checkmate / stalemate
- The extension hot-reloads on editor window focus after a rebuild (no editor restart needed)

### 4. Regenerate the clangd database (after C++ changes)

```powershell
cd client
scons platform=windows target=template_debug arch=x86_64 api_version=4.7 compiledb=yes compiledb_file=../compile_commands.json
```

Then restart the clangd language server in your editor.

### 5. Run the server (optional)

```powershell
cd server
scons
.\chess_server.exe          # listens on 127.0.0.1:8080 (Ctrl+C to stop)
.\tests\test_connection.ps1 # expect: 6 checks, 0 failures
```

The connection test starts and stops the server itself — don't run it while `chess_server.exe` is already listening on port 8080. Add `-v` to see step-by-step progress.

## Documentation

| Document | Audience | Contents |
|---|---|---|
| [AGENTS.md](AGENTS.md) | AI models | Invariants, file map, commands, gotchas |
| [docs/architecture.md](docs/architecture.md) | Humans + AI | Hexagonal layout, data flow, boundaries |
| [docs/core-api.md](docs/core-api.md) | Humans + AI | C++ domain API reference |
| [docs/facade-api.md](docs/facade-api.md) | Humans + AI | `ChessGame` GDExtension API and Dictionary contracts |
| [docs/scenes.md](docs/scenes.md) | Humans + AI | Scene tree, signals, script contracts |
| [docs/contributing.md](docs/contributing.md) | Humans + AI | Code style, layer rules, testing |
