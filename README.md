# media-player-cpp

OpenFrameworks media player with dual-buffer prefetch playback, optional subtitles, and an HTTP control API.

## Build

MSYS2 MinGW64 shell from the project folder:

```bash
make Release
cd bin && ./media-player-cpp.exe
```

`make Release` copies required MinGW64 runtime DLLs (including `glfw3.dll` and `libfreetype-6.dll`) into `bin/`, so the app also runs when launched from Explorer or PowerShell — not only inside the MSYS2 shell.

If you built before this step existed, run:

```bash
bash scripts/copy_msys2_dlls.sh
```

Place H.264 MP4 clips in `bin/data/`.

## HTTP control API

Default listen address: **`http://127.0.0.1:8080`** (localhost clients only).

All responses are JSON. Commands run on the main thread (same as the GUI).

### Endpoints

| GUI control | Method | Endpoint | Body |
|-------------|--------|----------|------|
| Status label | `GET` | `/api/status` | — |
| — | `GET` | `/api/health` | — |
| — | `GET` | `/api/clips` | — |
| Play | `POST` | `/api/play` | — |
| Stop | `POST` | `/api/stop` | — |
| Next Video | `POST` | `/api/next` | — |
| — | `POST` | `/api/previous` | — |
| Subtitles | `POST` | `/api/subtitles` | `{"enabled": true}` or `false` |
| — | `POST` | `/api/clips/{index}` | — (0-based index, opens paused preview) |

### Examples

```bash
curl http://127.0.0.1:8080/api/status
curl http://127.0.0.1:8080/api/clips
curl -X POST http://127.0.0.1:8080/api/play
curl -X POST http://127.0.0.1:8080/api/next
curl -X POST http://127.0.0.1:8080/api/stop
curl -X POST http://127.0.0.1:8080/api/subtitles -H "Content-Type: application/json" -d "{\"enabled\":true}"
curl -X POST http://127.0.0.1:8080/api/clips/0
```

### Sample status response

```json
{
  "loaded": true,
  "playing": true,
  "clipIndex": 0,
  "clipCount": 4,
  "clipName": "vid1.mp4",
  "subtitlesEnabled": false
}
```

## Architecture

```
HTTP / GUI  →  MediaPlayerController  →  VideoPanel
                                              ├── IClipSource (VideoClipLibrary)
                                              └── VideoPlaybackEngine
                                                    ├── async dual-slot prefetch
                                                    └── VideoRenderer (scaled GPU texture draw)
                                      ↘  SubtitlesOverlay
```

Copyright (c) vecnode 2026
