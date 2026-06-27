# CLAUDE.md

Guidance for Claude Code in this repository. The full agent/contributor guide is
in `AGENTS.md` — read it first.

@AGENTS.md

## Claude-specific quick reference

- This is an **openFrameworks** C++ media player (images/video, OCR subtitles,
  region framing) with an HTTP control API on `:8080`. It's an OF project under
  `apps/myApps/` (`OF_ROOT = ../../..`); addons: `ofxGui`, `ofxNetwork`.
- It is **app #3** of a three-app system, driven by the **metaagent** C++
  controller (`vecnode/metaagent`) over `/api/media/*`. metaagent can also build
  it (`make Release`) and run `bin/media-player-cpp.exe`, tracking the PID. Keep
  the HTTP routes in `src/HttpControlServer.cpp` stable and additive.
- **Build with MSYS2 MinGW64, not MSVC:** `make Release`, then
  `cd bin && ./media-player-cpp.exe` (the exe needs `bin/` as its cwd).
- **Threading rule:** never touch `ofVideoPlayer` off the main thread — the HTTP
  server executes commands via the polled `update()` on the GUI thread.
- **Don't** commit `bin/*` (except `bin/data/`), `obj/`, binaries, or
  media/corpus files (`*.md`, images, `*.pt`) — all git-ignored.
- Media/corpus + display behavior: `README.md`.
