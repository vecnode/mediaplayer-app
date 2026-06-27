# AGENTS.md — media-player-cpp

Guidance for AI coding agents (and humans) working in this repository. This file
is the canonical agent guide; `CLAUDE.md` defers to it.

## What this repository is

An **openFrameworks** (C++) media player for images and video, with optional OCR
subtitles, region framing from detection metadata, and an **HTTP control API**.
It is an OF *project*, expected to live under an openFrameworks install at
`apps/myApps/media-player-cpp` (the Makefile resolves `OF_ROOT = ../../..`).

Addons (`addons.make`): `ofxGui`, `ofxNetwork`.

### Role in the larger system

This is **app #3 of a three-app system**, controlled by **metaagent** (the C++
agent controller, repo `vecnode/metaagent`). metaagent drives playback over this
app's HTTP API and can build/run the process for centralised control:

- Playback: metaagent `POST /api/media/*` proxies to this app's API on `:8080`.
- Build:  metaagent `POST /api/media/build` runs the configured build command
  (`make Release`) in this project dir (`METAAGENT_MEDIA_PLAYER_DIR`).
- Run:    metaagent `POST /api/media/run` launches `bin/media-player-cpp.exe`
  (tracking its PID; stop via `POST /api/media/process/stop`).

App #2 is `vecnode/pre-training` (the LoRA adapter). This player does not call it
directly — metaagent coordinates them. The OCR/detection corpus this player
renders (`PDF_TEXT.md`, `OBJS_TEXT.md` in `bin/data/`) originates from that
pipeline / the UE plugin.

## Build & run

**MSYS2 MinGW64 shell**, from the project folder (OF on Windows is a MinGW
toolchain — do **not** build this with MSVC):

```bash
make Release
cd bin && ./media-player-cpp.exe        # or: make RunRelease
```

- The binary is `bin/media-player-cpp.exe`; it must run with `bin/` as the
  working directory so it finds `bin/data/`.
- `make Debug` / `make` builds the debug target.
- `make sync-corpus` (see `scripts/sync_metaagent_corpus.sh`) refreshes the
  metaagent corpus sources.

> When launched by metaagent, the build command and run binary are configurable
> (`METAAGENT_MEDIA_BUILD_CMD`, `METAAGENT_MEDIA_RUN_CMD`); the run process is
> started with `bin/` as its working directory.

## HTTP control API

`http://127.0.0.1:8080`, localhost-only (`HttpControlServer`, `ofxTCPServer`).
`update()` is polled from the main thread each frame, so commands run on the GUI
thread and `ofVideoPlayer` stays thread-safe.

| Action | Method | Endpoint |
| ------ | ------ | -------- |
| Status | GET  | `/api/status` |
| Playlist | GET | `/api/clips` |
| Next | POST | `/api/next` |
| Previous | POST | `/api/previous` |
| Play | POST | `/api/play` |
| Stop | POST | `/api/stop` |
| Subtitles | POST | `/api/subtitles` + `{"enabled": true}` |
| Open clip | POST | `/api/clips/{index}` |

Images are listed before videos. Play/Stop apply to video only. Keep this surface
stable — metaagent's media proxy depends on these exact routes and shapes.

## Code layout

```
src/
  main.cpp / ofApp.*            OF entry point + app shell
  MediaPlayerController.*       Command surface the HTTP server drives
  HttpControlServer.*           ofxNetwork JSON HTTP/1.1 API (port 8080)
  MediaPlaybackEngine.*         Video/image playback state
  MediaClipLibrary.* / IClipSource.h   Playlist + clip sources
  MediaCorpusProvider.*         Loads PDF_TEXT.md / OBJS_TEXT.md (OCR + regions)
  MediaPanel.* / MediaRenderer.*       Layout + draw (letterboxed, region framing)
  SubtitlesOverlay.*            On-screen OCR subtitle overlay
  PlatformVideo.* / MediaFoundationBackend.cpp   Platform video backend
  metaagent/                    Vendored metaagent core/media headers (shared types)
```

## Conventions & guardrails

- **C++ / openFrameworks idioms** — match the surrounding OF style (`ofApp`
  lifecycle, `of*` types). HTTP work stays in `HttpControlServer`; playback
  state in the engine/controller, not the server.
- **Threading:** never touch `ofVideoPlayer` off the main thread. The HTTP server
  only queues/executes via the polled `update()` on the GUI thread — preserve
  that.
- **Media data** lives in `bin/data/` and is **not** deleted by builds. Region
  framing + the green debug box require entries in `OBJS_TEXT.md` with
  `text_regions`; `PDF_TEXT.md` covers OCR-only subtitles.
- **Don't commit build output or media:** `bin/*` (except `bin/data/`), `obj/`,
  binaries, and image/video/`*.pt`/`*.md` corpus files are git-ignored — keep it
  that way.
- **Keep the HTTP API backward-compatible** with metaagent's `/api/media/*`
  proxy; if you add a route, add it additively.

See `README.md` for media-file/corpus details and the display behavior.
