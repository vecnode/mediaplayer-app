/*
 * media-player-cpp — PlatformVideo
 * Copyright (c) vecnode 2026
 */

#include "PlatformVideo.h"

#ifdef TARGET_WIN32
#include "ofMediaFoundationPlayer.h"
#endif

namespace PlatformVideo {

namespace {

#ifdef TARGET_WIN32
void tuneMediaFoundationPlayer(const std::shared_ptr<ofBaseVideoPlayer>& backend) {
	auto mf = std::dynamic_pointer_cast<ofMediaFoundationPlayer>(backend);
	if (!mf) {
		return;
	}

	// DX11 / DXGI shared texture path — avoids CPU pixel copies in ofVideoPlayer::update().
	mf->setUsingHWAccel(true);
}

bool hasPresentableFrame(const ofVideoPlayer& player) {
	return player.isLoaded()
		&& player.getWidth() > 0
		&& player.getHeight() > 0
		&& (player.isFrameNew() || player.getTexture().isAllocated());
}
#endif

} // namespace

void configurePlayer(ofVideoPlayer& player) {
	// Prefer the backend's internal GL/DX texture; ofVideoPlayer only uploads pixels when needed.
	player.setUseTexture(true);

#ifdef TARGET_WIN32
	// Non-const getPlayer() side-effects: it creates ofDirectShowPlayer when empty.
	const auto& backend = static_cast<const ofVideoPlayer&>(player).getPlayer();
	if (!std::dynamic_pointer_cast<ofMediaFoundationPlayer>(backend)) {
		auto mf = std::make_shared<ofMediaFoundationPlayer>();
		tuneMediaFoundationPlayer(mf);
		player.setPlayer(mf);
	} else {
		tuneMediaFoundationPlayer(backend);
	}
	ofLogNotice("PlatformVideo") << "Video backend: Media Foundation (Windows, HW texture path)";
#else
	ofLogNotice("PlatformVideo") << "Video backend: openFrameworks default";
#endif
}

of::filesystem::path loadPath(const of::filesystem::path& absolutePath) {
#ifdef TARGET_WIN32
	of::filesystem::path path = absolutePath;
	path.make_preferred();
	return path;
#else
	return ofToDataPathFS(absolutePath, true);
#endif
}

void primeFirstFrame(ofVideoPlayer& player) {
	if (!player.isLoaded()) {
		return;
	}

#ifdef TARGET_WIN32
	player.setPaused(true);

	// MF delivers the first frame asynchronously after Load().
	for (int i = 0; i < 120 && !hasPresentableFrame(player); ++i) {
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
