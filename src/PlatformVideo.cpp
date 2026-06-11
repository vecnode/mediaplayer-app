/*
 * media-player-cpp — PlatformVideo
 * Copyright (c) vecnode 2026
 */

#include "PlatformVideo.h"

#ifdef TARGET_WIN32
#include "ofMediaFoundationPlayer.h"
#endif

namespace PlatformVideo {

void configurePlayer(ofVideoPlayer& player) {
	player.setUseTexture(true);

#ifdef TARGET_WIN32
	// MSYS2 links DirectShow by default; MF handles H.264 mp4 natively on Windows 10+.
	player.setPlayer(std::make_shared<ofMediaFoundationPlayer>());
	ofLogNotice("PlatformVideo") << "Video backend: Media Foundation (Windows)";
#else
	ofLogNotice("PlatformVideo") << "Video backend: openFrameworks default";
#endif
}

of::filesystem::path loadPath(const of::filesystem::path& absolutePath) {
#ifdef TARGET_WIN32
	// Windows backends expect a full native path when launched from the project folder.
	return absolutePath;
#else
	// GStreamer / AVFoundation accept absolute paths; OF resolves them consistently.
	return ofToDataPathFS(absolutePath, true);
#endif
}

void primeFirstFrame(ofVideoPlayer& player) {
	if (!player.isLoaded()) {
		return;
	}

#ifdef TARGET_WIN32
	// Media Foundation rejects seek/firstFrame before playback has started.
	player.setPaused(true);
	for (int i = 0; i < 15; ++i) {
		player.update();
	}
#else
	player.stop();
	player.firstFrame();
	player.setPaused(true);
	for (int i = 0; i < 5; ++i) {
		player.update();
	}
#endif
}

void stopPlayback(ofVideoPlayer& player) {
	if (!player.isLoaded()) {
		return;
	}

#ifdef TARGET_WIN32
	player.setPaused(true);
#else
	player.stop();
	primeFirstFrame(player);
#endif
}

#ifdef TARGET_WIN32
void configureDataPathRoot() {
	const auto dataRoot = ofFilePath::getCurrentExeDirFS() / "data";
	ofSetDataPathRoot(dataRoot);
	ofLogNotice("PlatformVideo") << "Data root: " << dataRoot;
}
#endif

} // namespace PlatformVideo
