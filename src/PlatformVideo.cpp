/*
 * media-player-cpp — PlatformVideo
 * Copyright (c) vecnode 2026
 *
 * Windows playback uses Media Foundation with DX11 hardware decode and a shared
 * GPU texture path. Non-Windows builds use the openFrameworks default backend.
 */

#include "PlatformVideo.h"

#ifdef TARGET_WIN32
#include "ofMediaFoundationPlayer.h"
#endif

namespace PlatformVideo {

namespace {

#ifdef TARGET_WIN32
constexpr int kPrimeFramePollMax = 30; ///< Short poll in primeFirstFrame(); full decode continues in update().

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
	// IMPORTANT: non-const getPlayer() side-effects — it creates ofDirectShowPlayer when empty.
	// Always inspect the backend through the const overload before deciding to replace it.
	const auto& backend = static_cast<const ofVideoPlayer&>(player).getPlayer();
	if (!std::dynamic_pointer_cast<ofMediaFoundationPlayer>(backend)) {
		auto mf = std::make_shared<ofMediaFoundationPlayer>();
		tuneMediaFoundationPlayer(mf);
		player.setPlayer(mf);
	} else {
		tuneMediaFoundationPlayer(backend);
	}

	static bool loggedBackend = false;
	if (!loggedBackend) {
		ofLogNotice("PlatformVideo") << "Video backend: Media Foundation (Windows, HW texture path)";
		loggedBackend = true;
	}
#else
	static bool loggedBackend = false;
	if (!loggedBackend) {
		ofLogNotice("PlatformVideo") << "Video backend: openFrameworks default";
		loggedBackend = true;
	}
#endif
}

of::filesystem::path loadPath(const of::filesystem::path& absolutePath) {
#ifdef TARGET_WIN32
	// MF expects native absolute paths; ofToDataPathFS is unreliable when cwd != exe dir.
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
	// MF cannot seek before playback starts; pause and poll for the first decoded frame.
	player.setPaused(true);

	for (int i = 0; i < kPrimeFramePollMax && !hasPresentableFrame(player); ++i) {
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
	// Pause in place — MF stop()+seek(0) is slow and can fail before first play.
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
