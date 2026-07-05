/*
 * media-player-cpp — MediaPanel
 * Copyright (c) vecnode 2026
 */

#include "MediaPanel.h"

#include <algorithm>
#include <limits>

namespace {

std::string resolveDataDirectory() {
	return ofFilePath::getCurrentExeDir() + "data";
}

ofRectangle defaultMediaBounds() {
	constexpr float kMediaScale = 0.6f;
	const float w = static_cast<float>(ofGetWidth());
	const float h = static_cast<float>(ofGetHeight());
	const float mediaW = w * kMediaScale;
	const float mediaH = h * kMediaScale;
	return {(w - mediaW) * 0.5f, (h - mediaH) * 0.5f, mediaW, mediaH};
}

} // namespace

void MediaPanel::syncLoadedPath() {
	if (engine.isLoaded()) {
		loadedPath = engine.currentClip().displayName;
	} else {
		loadedPath.clear();
	}
}

void MediaPanel::refreshImageDrawHints(const ofRectangle& bounds) {
	lastDrawBounds_ = bounds;
	imageDrawHints_ = {};

	if (!engine.isCurrentClipImage() || loadedPath.empty()) {
		engine.setImageDrawHints(nullptr);
		return;
	}

	if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
		engine.setImageDrawHints(nullptr);
		return;
	}

	metaagent::media::IntRect regionBbox {};
	const bool hasRegion = corpus_.regionBboxForClip(
		loadedPath, selectedRegionIndex_, regionBbox);
	if (hasRegion && showRegionBBox_) {
		imageDrawHints_.has_debug_region = true;
		imageDrawHints_.debug_region_x = static_cast<float>(regionBbox.x);
		imageDrawHints_.debug_region_y = static_cast<float>(regionBbox.y);
		imageDrawHints_.debug_region_w = static_cast<float>(regionBbox.width);
		imageDrawHints_.debug_region_h = static_cast<float>(regionBbox.height);
	} else {
		const std::size_t regionCount = corpus_.regionCountForClip(loadedPath);
		if (regionCount == 0 && corpus_.findForClip(loadedPath) != nullptr) {
			ofLogWarning("MediaPanel") << "No OBJS regions for \"" << loadedPath
				<< "\" — PDF OCR exists but this file is missing from OBJS_TEXT.md "
				<< "(green debug box needs detected bboxes)";
		} else {
			ofLogWarning("MediaPanel") << "No corpus data for \"" << loadedPath
				<< "\" (not in PDF_TEXT.md / OBJS_TEXT.md)";
		}
	}

	imageDrawHints_.anim_mode = animationsEnabled_ ? selectedAnimMode_ : kAnimNone;
	imageDrawHints_.anim_start_seconds = animStartSeconds_;

	const float viewportAspect = bounds.width / bounds.height;
	metaagent::media::IntRect focus {};
	if (regionPanEnabled_ && hasRegion) {
		imageDrawHints_.pan_to_region = true;
		imageDrawHints_.pan_center_x = static_cast<float>(regionBbox.x)
			+ static_cast<float>(regionBbox.width) * 0.5f;
		imageDrawHints_.pan_center_y = static_cast<float>(regionBbox.y)
			+ static_cast<float>(regionBbox.height) * 0.5f;
	} else if (regionFocusEnabled_
		&& corpus_.focusRectForRegion(loadedPath, selectedRegionIndex_, viewportAspect, focus)) {
		imageDrawHints_.has_focus_rect = true;
		imageDrawHints_.cover_fit = true;
		imageDrawHints_.src_x = static_cast<float>(focus.x);
		imageDrawHints_.src_y = static_cast<float>(focus.y);
		imageDrawHints_.src_w = static_cast<float>(focus.width);
		imageDrawHints_.src_h = static_cast<float>(focus.height);
	} else if (hasRegion && regionFocusEnabled_) {
		ofLogWarning("MediaPanel") << "Focus crop failed for region "
			<< selectedRegionIndex_ << " on \"" << loadedPath << "\"";
	}

	engine.setImageDrawHints(&imageDrawHints_);
}

void MediaPanel::pickAnimationForSelection() {
	// Always animate - panning must never stop while a clip is on screen.
	// Randomly choose between the pan-only and pan+zoom styles; kAnimNone is
	// deliberately excluded here (it's only reachable via the master
	// "Animate" toggle, which forces kAnimNone for every clip when off).
	constexpr int kAnimatedModeCount = kAnimModeCount - 1;
	selectedAnimMode_ = kAnimDrift + static_cast<int>(ofRandom(static_cast<float>(kAnimatedModeCount)));
	selectedAnimMode_ = std::max(static_cast<int>(kAnimDrift), std::min(selectedAnimMode_, kAnimModeCount - 1));
	animStartSeconds_ = ofGetElapsedTimef();
}

void MediaPanel::onClipSwitched(const MediaPlaybackEngine::SwitchResult& result) {
	if (!result.success) {
		return;
	}

	syncLoadedPath();
	selectedRegionIndex_ = corpus_.pickRandomRegionIndex(loadedPath);
	pickAnimationForSelection();
	refreshImageDrawHints(lastDrawBounds_.width > 0.0f ? lastDrawBounds_ : defaultMediaBounds());

	if (clipChangedHandler) {
		clipChangedHandler();
	}
}

void MediaPanel::setClipChangedHandler(ClipChangedHandler handler) {
	clipChangedHandler = std::move(handler);
}

void MediaPanel::setup() {
	engine.setup();
	corpus_.setDataDirectory(resolveDataDirectory());

	engine.setSwitchHandler([this](const MediaPlaybackEngine::SwitchResult& result) {
		onClipSwitched(result);
	});

	library.scan();
	engine.attachClipSource(&library);

	if (library.empty()) {
		ofLogError("MediaPanel") << "No media found. Searched: " << library.getSearchLog();
		return;
	}

	if (!engine.openIndex(0, true)) {
		ofLogError("MediaPanel") << "Failed to load first media file";
		return;
	}

	syncLoadedPath();
	selectedRegionIndex_ = corpus_.pickRandomRegionIndex(loadedPath);
	pickAnimationForSelection();
	refreshImageDrawHints(lastDrawBounds_.width > 0.0f ? lastDrawBounds_ : defaultMediaBounds());
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

void MediaPanel::cycleRandom() {
	engine.skipToRandom();
}

bool MediaPanel::openClipAtIndex(std::size_t index, bool primePreviewFrame) {
	const bool opened = engine.openIndex(index, primePreviewFrame, true);
	if (opened) {
		syncLoadedPath();
		selectedRegionIndex_ = corpus_.pickRandomRegionIndex(loadedPath);
		pickAnimationForSelection();
		refreshImageDrawHints(lastDrawBounds_.width > 0.0f ? lastDrawBounds_ : defaultMediaBounds());
	}
	return opened;
}

void MediaPanel::update() {
	engine.update();
}

void MediaPanel::draw(const ofRectangle& bounds) const {
	auto* self = const_cast<MediaPanel*>(this);
	if (self->lastDrawBounds_.width != bounds.width
		|| self->lastDrawBounds_.height != bounds.height) {
		self->refreshImageDrawHints(bounds);
	}
	engine.draw(bounds);
}

void MediaPanel::setShowRegionBBox(bool show) {
	if (showRegionBBox_ == show) {
		return;
	}
	showRegionBBox_ = show;
	if (lastDrawBounds_.width > 0.0f) {
		refreshImageDrawHints(lastDrawBounds_);
	}
}

void MediaPanel::setRegionFocusEnabled(bool enabled) {
	if (regionFocusEnabled_ == enabled) {
		return;
	}
	regionFocusEnabled_ = enabled;
	if (enabled) {
		regionPanEnabled_ = false;
	}
	if (lastDrawBounds_.width > 0.0f) {
		refreshImageDrawHints(lastDrawBounds_);
	}
}

void MediaPanel::setRegionPanEnabled(bool enabled) {
	if (regionPanEnabled_ == enabled) {
		return;
	}
	regionPanEnabled_ = enabled;
	if (enabled) {
		regionFocusEnabled_ = false;
	}
	if (lastDrawBounds_.width > 0.0f) {
		refreshImageDrawHints(lastDrawBounds_);
	}
}

void MediaPanel::setAnimationsEnabled(bool enabled) {
	if (animationsEnabled_ == enabled) {
		return;
	}
	animationsEnabled_ = enabled;
	animStartSeconds_ = ofGetElapsedTimef();
	if (lastDrawBounds_.width > 0.0f) {
		refreshImageDrawHints(lastDrawBounds_);
	}
}
