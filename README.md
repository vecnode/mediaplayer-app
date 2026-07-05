# media-player-cpp

OpenFrameworks media player for images and video, with optional OCR subtitles and an HTTP control API.

## Media files

Put assets in **`bin/data/`** (PNG, JPG, MP4, etc.). Builds do not delete this folder.

| File | Purpose |
|------|---------|
| `PDF_TEXT.md` | OCR text per image (subtitles) |
| `OBJS_TEXT.md` | Detected regions + bboxes (**required** for region framing / green debug box) |

Region framing and the green debug outline only work for images listed in **`OBJS_TEXT.md`** with `text_regions`. `PDF_TEXT.md` can cover more files (OCR only) — those clips use width-fit display.

Images are listed before videos. **Next** / **Previous** cycle the playlist. **Play** / **Stop** apply to video only.

### Image display

When `OBJS_TEXT.md` has regions for an image, each clip shows **one detected area**, chosen **at random** on every switch. The view fills the window, scaled uniformly (no stretching), with black letterboxing as needed.

Without corpus data, images use width-fit framing.

While a clip is on screen it **never sits still**: the visible crop continuously pans on both axes (some clips read as mostly vertical, some mostly horizontal, some a diagonal mix — randomized per clip so consecutive clips look distinct), and about half the time also breathes in and out with a slow, never-ending zoom. Toggle the **Animate** control to turn this off entirely.

### Window

The window is fixed at **1920×1080** and is **not** drag-resizable. Press **F11** to toggle fullscreen (the "maximize" action for this window). The media view always fills the entire window/screen — no padding or centering.

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
