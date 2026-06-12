/*
 * media-player-cpp — VideoPanel
 * Copyright (c) vecnode 2026
 *
 * Thin facade over VideoClipLibrary + VideoPlaybackEngine.
 * ofApp depends on this class; extend playback by modifying the engine/library layers.
 */

#include "VideoPanel.h"

void VideoPanel::syncLoadedPath() {
	if (engine.isLoaded()) {
		loadedPath = engine.currentClip().displayName;
	} else {
		loadedPath.clear();
	}
}

void VideoPanel::setup() {
	engine.setup();

	library.scan();
	engine.attachLibrary(&library);

	if (library.empty()) {
		ofLogError("VideoPanel") << "No playable video found. Searched: " << library.getSearchLog();
		return;
	}

	if (!engine.openIndex(0, true)) {
		ofLogError("VideoPanel") << "Failed to load first video";
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
	const auto result = engine.skipToNext();
	if (result.success) {
		syncLoadedPath();
	}
}

bool VideoPanel::openClipAtIndex(std::size_t index, bool primePreviewFrame) {
	if (!engine.openIndex(index, primePreviewFrame)) {
		return false;
	}
	syncLoadedPath();
	return true;
}

void VideoPanel::update() {
	engine.update();
}

void VideoPanel::draw(const ofRectangle& bounds) const {
	engine.draw(bounds);
}
