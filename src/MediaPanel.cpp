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
	} else if (showRegionBBox_ && !hasRegion) {
		// Only worth diagnosing when the debug box is actually meant to be shown
		// and still couldn't find a region to draw — not just because the toggle
		// is off, which alone hits this branch on every clip regardless of data.
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
	imageDrawHints_.anim_amp_x = animAmpX_;
	imageDrawHints_.anim_amp_y = animAmpY_;
	imageDrawHints_.anim_freq_x = animFreqX_;
	imageDrawHints_.anim_freq_y = animFreqY_;
	imageDrawHints_.anim_phase_x = animPhaseX_;
	imageDrawHints_.anim_phase_y = animPhaseY_;
	imageDrawHints_.anim_zoom_amp = animZoomAmp_;
	imageDrawHints_.anim_zoom_freq = animZoomFreq_;
	imageDrawHints_.anim_zoom_phase = animZoomPhase_;

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

	imageDrawHints_.neighbor_prev2 = prevThumb2_.isAllocated() ? &prevThumb2_ : nullptr;
	imageDrawHints_.neighbor_prev1 = prevThumb1_.isAllocated() ? &prevThumb1_ : nullptr;
	imageDrawHints_.neighbor_next1 = nextThumb1_.isAllocated() ? &nextThumb1_ : nullptr;
	imageDrawHints_.neighbor_next2 = nextThumb2_.isAllocated() ? &nextThumb2_ : nullptr;

	engine.setImageDrawHints(&imageDrawHints_);
}

void MediaPanel::refreshNeighborThumbnails() {
	prevThumb1_.clear();
	prevThumb2_.clear();
	nextThumb1_.clear();
	nextThumb2_.clear();

	if (library.empty() || !engine.isCurrentClipImage() || loadedPath.empty()) {
		return;
	}

	const std::size_t currentIndex = engine.currentIndex();
	const std::size_t prev1Index = library.previousIndex(currentIndex);
	const std::size_t prev2Index = library.previousIndex(prev1Index);
	const std::size_t next1Index = library.nextIndex(currentIndex);
	const std::size_t next2Index = library.nextIndex(next1Index);

	auto loadIfImage = [&](std::size_t index, ofImage& out) {
		if (index == currentIndex) {
			return;
		}
		const MediaClip& clip = library.clipAt(index);
		if (clip.mediaType != ClipMediaType::Image || clip.absolutePath.empty()) {
			return;
		}
		out.load(clip.absolutePath);
	};

	loadIfImage(prev1Index, prevThumb1_);
	loadIfImage(prev2Index, prevThumb2_);
	loadIfImage(next1Index, nextThumb1_);
	loadIfImage(next2Index, nextThumb2_);
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

	// Randomize the motion character per clip so consecutive clips feel
	// visually distinct instead of repeating the exact same wobble: some
	// clips read as mostly vertical ("go down, go up"), some mostly
	// horizontal, some a genuinely diagonal mix of both. Amplitude is large
	// relative to the cross-canvas world (panels the same size as the
	// screen) so the camera actually swings out toward a neighbor rather
	// than just wobbling in place, and frequency is fast enough to feel
	// alive without being dizzying - a full swing takes roughly 10-18s,
	// so a clip shown for ~20s gets a full lap plus a bit more, with
	// constant motion the whole time.
	float ampX = ofRandom(0.35f, 0.60f);
	float ampY = ofRandom(0.35f, 0.60f);
	const int character = static_cast<int>(ofRandom(3.0f)); // 0 vertical, 1 horizontal, 2 diagonal
	if (character == 0) {
		ampX *= 0.4f;
		ampY *= 1.3f;
	} else if (character == 1) {
		ampX *= 1.3f;
		ampY *= 0.4f;
	}

	animAmpX_ = ampX;
	animAmpY_ = ampY;
	animFreqX_ = ofRandom(0.35f, 0.65f);
	animFreqY_ = ofRandom(0.30f, 0.60f);
	animPhaseX_ = ofRandom(0.0f, TWO_PI);
	animPhaseY_ = ofRandom(0.0f, TWO_PI);

	if (selectedAnimMode_ == kAnimSlowZoom) {
		animZoomAmp_ = ofRandom(0.15f, 0.30f);
		animZoomFreq_ = ofRandom(0.10f, 0.20f);
		animZoomPhase_ = ofRandom(0.0f, TWO_PI);
	} else {
		animZoomAmp_ = 0.0f;
	}
}

void MediaPanel::onClipSwitched(const MediaPlaybackEngine::SwitchResult& result) {
	if (!result.success) {
		return;
	}

	syncLoadedPath();
	selectedRegionIndex_ = corpus_.pickRandomRegionIndex(loadedPath);
	pickAnimationForSelection();
	refreshNeighborThumbnails();
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
	refreshNeighborThumbnails();
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
		refreshNeighborThumbnails();
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
