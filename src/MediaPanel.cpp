/*
 * media-player-cpp — MediaPanel
 * Copyright (c) vecnode 2026
 */

#include "MediaPanel.h"

void MediaPanel::syncLoadedPath() {
	if (engine.isLoaded()) {
		loadedPath = engine.currentClip().displayName;
	} else {
		loadedPath.clear();
	}
}

void MediaPanel::onClipSwitched(const MediaPlaybackEngine::SwitchResult& result) {
	if (!result.success) {
		return;
	}

	syncLoadedPath();

	if (clipChangedHandler) {
		clipChangedHandler();
	}
}

void MediaPanel::setClipChangedHandler(ClipChangedHandler handler) {
	clipChangedHandler = std::move(handler);
}

void MediaPanel::setup() {
	engine.setup();

	library.scan();
	engine.attachClipSource(&library);
	engine.setSwitchHandler([this](const MediaPlaybackEngine::SwitchResult& result) {
		onClipSwitched(result);
	});

	if (library.empty()) {
		ofLogError("MediaPanel") << "No media found. Searched: " << library.getSearchLog();
		return;
	}

	if (!engine.openIndex(0, true)) {
		ofLogError("MediaPanel") << "Failed to load first media file";
		return;
	}

	syncLoadedPath();
}

void MediaPanel::play() {
	engine.play();
}

void MediaPanel::stop() {
	engine.stop();
}

void MediaPanel::cycleNext() {
	engine.skipToNext();
}

void MediaPanel::cyclePrevious() {
	engine.skipToPrevious();
}

bool MediaPanel::openClipAtIndex(std::size_t index, bool primePreviewFrame) {
	return engine.openIndex(index, primePreviewFrame, true);
}

void MediaPanel::update() {
	engine.update();
}

void MediaPanel::draw(const ofRectangle& bounds) const {
	engine.draw(bounds);
}
