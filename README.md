# media-player-cpp

OpenFrameworks media player for **images and video** with dual-buffer prefetch, optional subtitles, and an HTTP control API.

## Media files

Put your dataset in **`bin/data/`** (PNG, JPG, MP4, etc.). The build and `make RunRelease` **do not delete or overwrite** this folder — only the exe and DLLs in `bin/` are updated.

Images are listed **before** videos in the playlist. **Next** cycles through all media; **Play/Stop** apply to video only.

## Build

MSYS2 MinGW64 shell from the project folder:

```bash
make Release
cd bin && ./media-player-cpp.exe
```

Or use VS Code **Build and Run Release**.

## HTTP control API

Default: **`http://127.0.0.1:8080`** (localhost only).

| Control | Method | Endpoint |
|---------|--------|----------|
| Status | `GET` | `/api/status` |
| Playlist | `GET` | `/api/clips` |
| Next | `POST` | `/api/next` |
| Previous | `POST` | `/api/previous` |
| Play (video) | `POST` | `/api/play` |
| Stop (video) | `POST` | `/api/stop` |
| Subtitles | `POST` | `/api/subtitles` + `{"enabled": true}` |
| Open by index | `POST` | `/api/clips/{index}` |

## Architecture

```
HTTP / GUI  →  MediaPlayerController  →  MediaPanel
                                              ├── MediaClipLibrary (images + video)
                                              └── MediaPlaybackEngine
                                                    └── MediaRenderer
                                      ↘  SubtitlesOverlay
```

Copyright (c) vecnode 2026
