/*
 * media-player-cpp — VideoPanel
 * Copyright (c) vecnode 2026
 */

#include "VideoPanel.h"

void VideoPanel::syncLoadedPath() {
	if (engine.isLoaded()) {
		loadedPath = engine.currentClip().displayName;
	} else {
		loadedPath.clear();
	}
}

void VideoPanel::onClipSwitched(const VideoPlaybackEngine::SwitchResult& result) {
	if (!result.success) {
		return;
	}

	syncLoadedPath();

	if (clipChangedHandler) {
		clipChangedHandler();
	}
}

void VideoPanel::setClipChangedHandler(ClipChangedHandler handler) {
	clipChangedHandler = std::move(handler);
}

void VideoPanel::setup() {
	engine.setup();

	library.scan();
	engine.attachClipSource(&library);
	engine.setSwitchHandler([this](const VideoPlaybackEngine::SwitchResult& result) {
		onClipSwitched(result);
	});

	if (library.empty()) {
		ofLogError("VideoPanel") << "No playable media found. Searched: " << library.getSearchLog();
		return;
	}

	if (!engine.openIndex(0, true)) {
		ofLogError("VideoPanel") << "Failed to load first media file";
		return;
	}

	syncLoadedPath();
}

void VideoPanel::play() {
	engine.play();
}

void VideoPanel::stop() {
	engine.stop();
}

void VideoPanel::cycleNext() {
	engine.skipToNext();
}

void VideoPanel::cyclePrevious() {
	engine.skipToPrevious();
}

bool VideoPanel::openClipAtIndex(std::size_t index, bool primePreviewFrame) {
	return engine.openIndex(index, primePreviewFrame, true);
}

void VideoPanel::update() {
	engine.update();
}

void VideoPanel::draw(const ofRectangle& bounds) const {
	engine.draw(bounds);
}
