/*
 * media-player-cpp — Media Foundation backend
 * Copyright (c) vecnode 2026
 *
 * Compiled only on Windows. MSYS2 openFrameworks excludes Media Foundation from
 * the core library; we compile it with the app for native H.264 mp4 playback.
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
