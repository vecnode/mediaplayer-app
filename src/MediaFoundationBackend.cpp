/*
 * media-player-cpp — Media Foundation backend
 * Copyright (c) vecnode 2026
 *
 * Compiled only on Windows. MSYS2 openFrameworks excludes Media Foundation from
 * the core library; we compile the OF sources here and link D3D11/DXGI/XAudio2
 * in config.make so H.264 MP4 plays without external codec packs.
 *
 * ofMain.h must come first so TARGET_WIN32 is defined before the #ifdef below.
 */

#include "ofMain.h"

#ifdef TARGET_WIN32

#include <windows.h>
#include <oleauto.h>

#ifndef SysReleaseString
#define SysReleaseString(str) SysFreeString(str)
#endif

#include "../../../libs/openFrameworks/sound/ofMediaFoundationSoundPlayer.cpp"
#include "../../../libs/openFrameworks/video/ofMediaFoundationPlayer.cpp"

#endif // TARGET_WIN32
