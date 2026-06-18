/*
 * media-player-cpp — MediaPanel
 * Copyright (c) vecnode 2026
 */

#include "MediaPanel.h"

namespace {

std::string resolveDataDirectory() {
	return ofFilePath::getCurrentExeDir() + "data";
}

} // namespace

void MediaPanel::syncLoadedPath() {
	if (engine.isLoaded()) {
		loadedPath = engine.currentClip().displayName;
	} else {
		loadedPath.clear();
	}
}

void MediaPanel::refreshImageDrawHints() {
	imageDrawHints_ = {};

	if (!engine.isCurrentClipImage() || loadedPath.empty()) {
		engine.setImageDrawHints(nullptr);
		return;
	}

	metaagent::media::IntRect focus {};
	if (!corpus_.focusRectForClip(loadedPath, engine.currentIndex(), focus)) {
		engine.setImageDrawHints(nullptr);
		return;
	}

	imageDrawHints_.has_focus_rect = true;
	imageDrawHints_.cover_fit = true;
	imageDrawHints_.src_x = static_cast<float>(focus.x);
	imageDrawHints_.src_y = static_cast<float>(focus.y);
	imageDrawHints_.src_w = static_cast<float>(focus.width);
	imageDrawHints_.src_h = static_cast<float>(focus.height);
	engine.setImageDrawHints(&imageDrawHints_);
}

void MediaPanel::onClipSwitched(const MediaPlaybackEngine::SwitchResult& result) {
	if (!result.success) {
		return;
	}

	syncLoadedPath();
	refreshImageDrawHints();

	if (clipChangedHandler) {
		clipChangedHandler();
	}
}

void MediaPanel::setClipChangedHandler(ClipChangedHandler handler) {
	clipChangedHandler = std::move(handler);
}

void MediaPanel::setup() {
	engine.setup();
	corpus_.setup(resolveDataDirectory());

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
	refreshImageDrawHints();
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
	const bool opened = engine.openIndex(index, primePreviewFrame, true);
	if (opened) {
		syncLoadedPath();
		refreshImageDrawHints();
	}
	return opened;
}

void MediaPanel::update() {
	engine.update();
}

void MediaPanel::draw(const ofRectangle& bounds) const {
	engine.draw(bounds);
}
