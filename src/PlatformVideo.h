#pragma once

#include "ofMain.h"
#include "ofVideoPlayer.h"

/// Platform-specific video player setup and playback helpers.
namespace PlatformVideo {

/// Select the best backend and enable GPU texture upload (once per ofVideoPlayer).
void configurePlayer(ofVideoPlayer& player);

/// Path to pass to ofVideoPlayer::load() for a discovered absolute file.
of::filesystem::path loadPath(const of::filesystem::path& absolutePath);

/// Decode and hold frame 0 while paused.
void primeFirstFrame(ofVideoPlayer& player);

/// Stop playback and return to a paused preview frame.
void stopPlayback(ofVideoPlayer& player);

#ifdef TARGET_WIN32
/// Anchor bin/data to the folder beside the .exe (MSYS2 cwd may be the project root).
void configureDataPathRoot();
#endif

} // namespace PlatformVideo
