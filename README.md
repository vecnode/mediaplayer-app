# mediaplayer-cpp

![Language](https://img.shields.io/badge/language-C%2B%2B-blue.svg)
![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)

OpenFrameworks media player for images and video, with optional OCR subtitles and an HTTP control API.

## Build

MSYS2 MinGW64 shell from the project folder:

```bash
make Release
cd bin && ./media-player-cpp.exe
```

Optional — refresh metaagent corpus sources after they change in the UE plugin:

```bash
make sync-corpus
```

## HTTP API

`http://127.0.0.1:8080` (localhost)

| Action | Method | Endpoint |
|--------|--------|----------|
| Status | GET | `/api/status` |
| Playlist | GET | `/api/clips` |
| Next | POST | `/api/next` |
| Previous | POST | `/api/previous` |
| Play | POST | `/api/play` |
| Stop | POST | `/api/stop` |
| Subtitles | POST | `/api/subtitles` + `{"enabled": true}` |
| Open clip | POST | `/api/clips/{index}` |

Copyright (c) vecnode 2026
