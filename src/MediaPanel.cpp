/*
 * media-player-cpp — MediaPanel
 * Copyright (c) vecnode 2026
 */

#include "MediaPanel.h"

#include <algorithm>
#include <limits>
#include <utility>

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

/// Finds up to `maxCount` axis-aligned rectangles (full-image pixel coords)
/// that are genuinely blank page background, via a coarse grid scan sampling
/// actual pixel brightness - not a proxy via OCR text-region boxes, which
/// only box specific labeled fields (e.g. "Subject:") and leave real
/// paragraph text completely unmarked, letting overlays land right on top of
/// visible content. Sorted largest-first; merges contiguous blank cells
/// along each row into wider candidates.
std::vector<metaagent::media::IntRect> computeWhiteAreaRects(const ofPixels& pixels, std::size_t maxCount) {
	std::vector<metaagent::media::IntRect> result;

	const int width = static_cast<int>(pixels.getWidth());
	const int height = static_cast<int>(pixels.getHeight());
	if (width <= 0 || height <= 0) {
		return result;
	}

	constexpr int kGridCols = 8;
	constexpr int kGridRows = 10;
	constexpr int kSampleStride = 4;
	constexpr unsigned char kWhiteChannelThreshold = 235;
	constexpr float kBlankCellFraction = 0.94f;

	const int cellW = width / kGridCols;
	const int cellH = height / kGridRows;
	if (cellW <= 0 || cellH <= 0) {
		return result;
	}

	bool blank[kGridRows][kGridCols] = {};
	for (int r = 0; r < kGridRows; ++r) {
		for (int c = 0; c < kGridCols; ++c) {
			const int startX = c * cellW;
			const int startY = r * cellH;
			const int endX = std::min(width, startX + cellW);
			const int endY = std::min(height, startY + cellH);

			int sampleCount = 0;
			int whiteCount = 0;
			for (int y = startY; y < endY; y += kSampleStride) {
				for (int x = startX; x < endX; x += kSampleStride) {
					const ofColor color = pixels.getColor(x, y);
					++sampleCount;
					if (color.r >= kWhiteChannelThreshold
						&& color.g >= kWhiteChannelThreshold
						&& color.b >= kWhiteChannelThreshold) {
						++whiteCount;
					}
				}
			}

			blank[r][c] = sampleCount > 0
				&& (static_cast<float>(whiteCount) / static_cast<float>(sampleCount)) >= kBlankCellFraction;
		}
	}

	for (int r = 0; r < kGridRows; ++r) {
		int c = 0;
		while (c < kGridCols) {
			if (!blank[r][c]) {
				++c;
				continue;
			}
			const int startCol = c;
			while (c < kGridCols && blank[r][c]) {
				++c;
			}

			metaagent::media::IntRect rect;
			rect.x = startCol * cellW;
			rect.y = r * cellH;
			rect.width = (c - startCol) * cellW;
			rect.height = cellH;
			result.push_back(rect);
		}
	}

	std::sort(result.begin(), result.end(), [](const metaagent::media::IntRect& a, const metaagent::media::IntRect& b) {
		return (static_cast<int64_t>(a.width) * a.height) > (static_cast<int64_t>(b.width) * b.height);
	});
	if (result.size() > maxCount) {
		result.resize(maxCount);
	}
	return result;
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

	for (std::size_t i = 0; i < neighborOverlayAssignments_.size()
		&& i < static_cast<std::size_t>(ImageDrawHints::kMaxNeighborOverlays); ++i) {
		ImageDrawHints::NeighborOverlaySlot& slot = imageDrawHints_.neighbor_overlays[i];
		const NeighborOverlayAssignment& assignment = neighborOverlayAssignments_[i];
		slot.has_rect = true;
		slot.rect_x = static_cast<float>(assignment.rect.x);
		slot.rect_y = static_cast<float>(assignment.rect.y);
		slot.rect_w = static_cast<float>(assignment.rect.width);
		slot.rect_h = static_cast<float>(assignment.rect.height);
		slot.thumb = assignment.thumb;
		slot.rotation_deg = assignment.rotationDeg;
	}

	engine.setImageDrawHints(&imageDrawHints_);
}

void MediaPanel::refreshNeighborOverlays() {
	neighborOverlayAssignments_.clear();
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

	auto loadIfImage = [&](std::size_t index, ofImage& out) -> const ofImage* {
		if (index == currentIndex) {
			return nullptr;
		}
		const MediaClip& clip = library.clipAt(index);
		if (clip.mediaType != ClipMediaType::Image || clip.absolutePath.empty()) {
			return nullptr;
		}
		return out.load(clip.absolutePath) ? &out : nullptr;
	};

	std::vector<const ofImage*> images = {
		loadIfImage(prev1Index, prevThumb1_),
		loadIfImage(next1Index, nextThumb1_),
		loadIfImage(prev2Index, prevThumb2_),
		loadIfImage(next2Index, nextThumb2_),
	};
	images.erase(std::remove(images.begin(), images.end(), nullptr), images.end());

	// Randomly occupy the blank spots rather than a fixed prev/next priority
	// order, so which neighbor lands where varies clip to clip.
	for (std::size_t i = images.size(); i > 1; --i) {
		const std::size_t j = static_cast<std::size_t>(ofRandom(static_cast<float>(i)));
		std::swap(images[i - 1], images[j]);
	}

	// Real pixel-based blank-area detection (not an OCR-region proxy, which
	// only boxes specific labeled fields and leaves real paragraph text
	// unmarked - that let overlays land on top of visible content).
	const std::string& currentAbsolutePath = library.clipAt(currentIndex).absolutePath;
	ofPixels currentPixels;
	std::vector<metaagent::media::IntRect> rects;
	if (!currentAbsolutePath.empty() && ofLoadImage(currentPixels, currentAbsolutePath)) {
		rects = computeWhiteAreaRects(currentPixels, static_cast<std::size_t>(ImageDrawHints::kMaxNeighborOverlays));
	}

	// Small tilt, chosen once per assignment (never re-rolled per frame) so the
	// collage reads as tastefully scattered prints rather than jittering.
	constexpr float kMaxTiltDeg = 6.0f;

	std::size_t rectIndex = 0;
	for (const ofImage* image : images) {
		if (rectIndex >= rects.size()) {
			break;
		}
		neighborOverlayAssignments_.push_back(
			{rects[rectIndex], image, ofRandom(-kMaxTiltDeg, kMaxTiltDeg)});
		++rectIndex;
	}
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
	// horizontal, some a diagonal mix of both.
	float ampX = ofRandom(0.05f, 0.10f);
	float ampY = ofRandom(0.05f, 0.10f);
	const int character = static_cast<int>(ofRandom(3.0f)); // 0 vertical, 1 horizontal, 2 mixed
	if (character == 0) {
		ampX *= 0.2f;
		ampY *= 1.5f;
	} else if (character == 1) {
		ampX *= 1.5f;
		ampY *= 0.2f;
	}

	animAmpX_ = ampX;
	animAmpY_ = ampY;
	animFreqX_ = ofRandom(0.16f, 0.36f);
	animFreqY_ = ofRandom(0.14f, 0.32f);
	animPhaseX_ = ofRandom(0.0f, TWO_PI);
	animPhaseY_ = ofRandom(0.0f, TWO_PI);

	if (selectedAnimMode_ == kAnimSlowZoom) {
		animZoomAmp_ = ofRandom(0.08f, 0.18f);
		animZoomFreq_ = ofRandom(0.05f, 0.12f);
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
	refreshNeighborOverlays();
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
	refreshNeighborOverlays();
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
		refreshNeighborOverlays();
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
