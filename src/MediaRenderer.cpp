/*
 * media-player-cpp — MediaRenderer
 * Copyright (c) vecnode 2026
 */

#include "MediaRenderer.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

struct WidthFitLayout {
	ofRectangle dest;
	float srcX = 0.0f;
	float srcY = 0.0f;
	float srcW = 0.0f;
	float srcH = 0.0f;
};

WidthFitLayout widthFitLayout(float mediaW, float mediaH, const ofRectangle& bounds) {
	WidthFitLayout layout;

	if (mediaW <= 0.0f || mediaH <= 0.0f || bounds.width <= 0.0f || bounds.height <= 0.0f) {
		layout.dest = bounds;
		layout.srcW = mediaW;
		layout.srcH = mediaH;
		return layout;
	}

	const float scale = bounds.width / mediaW;
	const float fullDrawH = mediaH * scale;

	layout.dest.x = bounds.x;
	layout.dest.width = bounds.width;
	layout.srcX = 0.0f;
	layout.srcW = mediaW;

	if (fullDrawH <= bounds.height) {
		layout.dest.height = fullDrawH;
		layout.dest.y = bounds.y + (bounds.height - fullDrawH) * 0.5f;
		layout.srcY = 0.0f;
		layout.srcH = mediaH;
	} else {
		layout.dest.y = bounds.y;
		layout.dest.height = bounds.height;

		const float visibleMediaH = bounds.height / scale;
		layout.srcY = (mediaH - visibleMediaH) * 0.5f;
		layout.srcH = visibleMediaH;
	}

	return layout;
}

WidthFitLayout widthFitLayoutOnPoint(
	float mediaW,
	float mediaH,
	const ofRectangle& bounds,
	float centerX,
	float centerY) {
	WidthFitLayout layout;

	if (mediaW <= 0.0f || mediaH <= 0.0f || bounds.width <= 0.0f || bounds.height <= 0.0f) {
		layout.dest = bounds;
		layout.srcW = mediaW;
		layout.srcH = mediaH;
		return layout;
	}

	const float scale = bounds.width / mediaW;
	const float fullDrawH = mediaH * scale;

	layout.dest.x = bounds.x;
	layout.dest.width = bounds.width;
	layout.srcX = 0.0f;
	layout.srcW = mediaW;

	if (fullDrawH <= bounds.height) {
		layout.dest.height = fullDrawH;
		layout.dest.y = bounds.y + (bounds.height - fullDrawH) * 0.5f;
		layout.srcY = 0.0f;
		layout.srcH = mediaH;
		return layout;
	}

	layout.dest.y = bounds.y;
	layout.dest.height = bounds.height;

	const float visibleMediaH = bounds.height / scale;
	layout.srcH = visibleMediaH;
	layout.srcY = centerY - visibleMediaH * 0.5f;
	layout.srcY = std::max(0.0f, std::min(layout.srcY, mediaH - visibleMediaH));

	return layout;
}

ofRectangle mapImageRectToScreen(
	const ofRectangle& dest,
	float cropSrcX,
	float cropSrcY,
	float cropSrcW,
	float cropSrcH,
	float rectX,
	float rectY,
	float rectW,
	float rectH) {
	if (cropSrcW <= 0.0f || cropSrcH <= 0.0f || rectW <= 0.0f || rectH <= 0.0f) {
		return {};
	}

	const float scaleX = dest.width / cropSrcW;
	const float scaleY = dest.height / cropSrcH;
	return {
		dest.x + (rectX - cropSrcX) * scaleX,
		dest.y + (rectY - cropSrcY) * scaleY,
		rectW * scaleX,
		rectH * scaleY
	};
}

void drawDebugRegionOutline(
	const ofRectangle& screenRect) {
	if (screenRect.width <= 0.0f || screenRect.height <= 0.0f) {
		return;
	}

	ofPushStyle();
	ofSetColor(0, 255, 0);
	ofSetLineWidth(6.0f);
	ofNoFill();
	ofDrawRectangle(screenRect);
	ofPopStyle();
}

void drawDebugRegionForLayout(
	const ImageDrawHints& hints,
	const ofRectangle& dest,
	float cropSrcX,
	float cropSrcY,
	float cropSrcW,
	float cropSrcH) {
	if (!hints.has_debug_region) {
		return;
	}

	const ofRectangle screenRect = mapImageRectToScreen(
		dest,
		cropSrcX, cropSrcY, cropSrcW, cropSrcH,
		hints.debug_region_x, hints.debug_region_y,
		hints.debug_region_w, hints.debug_region_h);
	drawDebugRegionOutline(screenRect);
}

/// Cover-fits the `cropX/Y/W/H` region of `thumb` (its own pixel coords) into
/// `screenRect` (scales to fill, cropping overflow on one axis, centered) -
/// a zoomed-in cut of one piece of the neighbor image, not the whole page.
/// No animation of its own since it inherits the host rect's motion for free
/// (the destination rect is mapped through the current image's own pan/zoom
/// crop every frame).
void drawCoverFitFromRect(
	const ofImage& thumb, const ofRectangle& screenRect,
	float cropX, float cropY, float cropW, float cropH) {
	if (cropW <= 0.0f || cropH <= 0.0f) {
		return;
	}

	const float destAspect = screenRect.width / screenRect.height;
	const float cropAspect = cropW / cropH;

	float srcW = cropW;
	float srcH = cropH;
	if (cropAspect > destAspect) {
		srcH = cropH;
		srcW = cropH * destAspect;
	} else {
		srcW = cropW;
		srcH = cropW / destAspect;
	}
	const float srcX = cropX + (cropW - srcW) * 0.5f;
	const float srcY = cropY + (cropH - srcH) * 0.5f;

	thumb.drawSubsection(screenRect.x, screenRect.y, screenRect.width, screenRect.height, srcX, srcY, srcW, srcH);
}

// Hard cap on each overlay's on-screen footprint - small "photo print"
// snippets, never a large patch, however big the blank area behind them is.
constexpr float kNeighborOverlayMaxPx = 50.0f;
constexpr float kNeighborOverlayBorderPx = 3.0f;
constexpr float kNeighborOverlayShadowOffsetPx = 2.5f;

/// Draws each neighbor-clip overlay (if present) mapped from full-image pixel
/// coordinates into the current on-screen crop window, so overlays stay
/// glued to their assigned blank spot as the current image pans/zooms. Each
/// is rendered as a small (<= 50x50px), bordered, drop-shadowed, gently
/// tilted print - a tasteful collage accent, never competing with the
/// current image for attention.
void drawNeighborOverlays(
	const ImageDrawHints* hints,
	const ofRectangle& dest,
	float cropSrcX, float cropSrcY, float cropSrcW, float cropSrcH) {
	if (!hints || cropSrcW <= 0.0f || cropSrcH <= 0.0f) {
		return;
	}

	for (const auto& slot : hints->neighbor_overlays) {
		if (!slot.has_rect || !slot.thumb || !slot.thumb->isAllocated()) {
			continue;
		}

		const ofRectangle mapped = mapImageRectToScreen(
			dest, cropSrcX, cropSrcY, cropSrcW, cropSrcH,
			slot.rect_x, slot.rect_y, slot.rect_w, slot.rect_h);
		if (mapped.width <= 2.0f || mapped.height <= 2.0f) {
			continue;
		}

		const float tileW = std::min(mapped.width, kNeighborOverlayMaxPx);
		const float tileH = std::min(mapped.height, kNeighborOverlayMaxPx);
		const float centerX = mapped.x + mapped.width * 0.5f;
		const float centerY = mapped.y + mapped.height * 0.5f;
		const float halfW = tileW * 0.5f;
		const float halfH = tileH * 0.5f;

		ofPushStyle();
		ofPushMatrix();
		ofTranslate(centerX, centerY);
		ofRotateDeg(slot.rotation_deg);

		ofFill();
		ofSetColor(0, 0, 0, 80);
		ofDrawRectangle(
			-halfW - kNeighborOverlayBorderPx + kNeighborOverlayShadowOffsetPx,
			-halfH - kNeighborOverlayBorderPx + kNeighborOverlayShadowOffsetPx,
			tileW + kNeighborOverlayBorderPx * 2.0f,
			tileH + kNeighborOverlayBorderPx * 2.0f);

		ofSetColor(255);
		ofDrawRectangle(
			-halfW - kNeighborOverlayBorderPx, -halfH - kNeighborOverlayBorderPx,
			tileW + kNeighborOverlayBorderPx * 2.0f, tileH + kNeighborOverlayBorderPx * 2.0f);

		drawCoverFitFromRect(
			*slot.thumb, {-halfW, -halfH, tileW, tileH},
			slot.source_crop_x, slot.source_crop_y, slot.source_crop_w, slot.source_crop_h);

		ofNoFill();
		ofSetColor(0, 0, 0, 70);
		ofSetLineWidth(1.0f);
		ofDrawRectangle(-halfW, -halfH, tileW, tileH);

		ofPopMatrix();
		ofPopStyle();
	}
}

void drawWidthFitMedia(const ofRectangle& bounds, float mediaW, float mediaH,
	const std::function<void(const WidthFitLayout&)>& drawMedia) {
	ofSetColor(0);
	ofDrawRectangle(bounds);

	const WidthFitLayout layout = widthFitLayout(mediaW, mediaH, bounds);
	ofSetColor(255);
	drawMedia(layout);
}

/// Applies the selection animation to a source crop window, keeping it inside
/// the full image. Pan runs on both axes continuously for as long as the clip
/// is shown - it never settles into a resting state. Amplitude/frequency/phase
/// come from the hints (randomized per clip in
/// MediaPanel::pickAnimationForSelection), so one clip might read as mostly
/// vertical ("go down, go up"), another mostly horizontal, another a diagonal
/// mix - kAnimSlowZoom layers a never-settling breathing zoom on top of the
/// same continuous pan.
void animateSrcRect(const ImageDrawHints& hints,
	float fullW, float fullH,
	float& srcX, float& srcY, float& srcW, float& srcH) {
	if (hints.anim_mode == kAnimNone || srcW <= 0.0f || srcH <= 0.0f) {
		return;
	}

	const float t = ofGetElapsedTimef() - hints.anim_start_seconds;

	srcX += hints.anim_amp_x * srcW * std::sin(t * hints.anim_freq_x + hints.anim_phase_x);
	srcY += hints.anim_amp_y * srcH * std::cos(t * hints.anim_freq_y + hints.anim_phase_y);

	if (hints.anim_mode == kAnimSlowZoom && hints.anim_zoom_amp > 0.0f) {
		// Zoom oscillates forever (never eases to a fixed end state and stops).
		const float zoomWave = 0.5f + 0.5f * std::sin(t * hints.anim_zoom_freq + hints.anim_zoom_phase);
		const float factor = 1.0f - hints.anim_zoom_amp * zoomWave;
		const float centerX = srcX + srcW * 0.5f;
		const float centerY = srcY + srcH * 0.5f;
		srcW *= factor;
		srcH *= factor;
		srcX = centerX - srcW * 0.5f;
		srcY = centerY - srcH * 0.5f;
	}

	srcX = std::max(0.0f, std::min(srcX, fullW - srcW));
	srcY = std::max(0.0f, std::min(srcY, fullH - srcH));
}

} // namespace

void MediaRenderer::draw(const ofVideoPlayer& player, const ofRectangle& bounds) {
	if (!player.isLoaded()) {
		return;
	}

	const float mediaW = static_cast<float>(player.getWidth());
	const float mediaH = static_cast<float>(player.getHeight());

	drawWidthFitMedia(bounds, mediaW, mediaH, [&](const WidthFitLayout& layout) {
		const ofTexture& tex = player.getTexture();
		if (tex.isAllocated()) {
			tex.drawSubsection(
				layout.dest.x, layout.dest.y, layout.dest.width, layout.dest.height,
				layout.srcX, layout.srcY, layout.srcW, layout.srcH);
			return;
		}

		player.draw(layout.dest.x, layout.dest.y, layout.dest.width, layout.dest.height);
	});
}

void MediaRenderer::draw(const ofImage& image, const ofRectangle& bounds, const ImageDrawHints* hints) {
	if (!image.isAllocated()) {
		return;
	}

	const float fullW = static_cast<float>(image.getWidth());
	const float fullH = static_cast<float>(image.getHeight());
	float mediaW = fullW;
	float mediaH = fullH;
	float srcX = 0.0f;
	float srcY = 0.0f;
	float srcW = mediaW;
	float srcH = mediaH;

	const bool use_focus = hints && hints->has_focus_rect && hints->src_w > 0.0f && hints->src_h > 0.0f;
	if (use_focus) {
		srcX = hints->src_x;
		srcY = hints->src_y;
		srcW = hints->src_w;
		srcH = hints->src_h;
		mediaW = srcW;
		mediaH = srcH;
	}

	ofSetColor(0);
	ofDrawRectangle(bounds);
	ofSetColor(255);

	if (use_focus && hints->cover_fit) {
		animateSrcRect(*hints, fullW, fullH, srcX, srcY, srcW, srcH);
		image.drawSubsection(
			bounds.x, bounds.y, bounds.width, bounds.height,
			srcX, srcY, srcW, srcH);

		drawDebugRegionForLayout(*hints, bounds, srcX, srcY, srcW, srcH);
		drawNeighborOverlays(hints, bounds, srcX, srcY, srcW, srcH);
		return;
	}

	if (hints && hints->pan_to_region) {
		WidthFitLayout layout = widthFitLayoutOnPoint(
			fullW, fullH, bounds, hints->pan_center_x, hints->pan_center_y);
		animateSrcRect(*hints, fullW, fullH, layout.srcX, layout.srcY, layout.srcW, layout.srcH);
		image.drawSubsection(
			layout.dest.x, layout.dest.y, layout.dest.width, layout.dest.height,
			layout.srcX, layout.srcY, layout.srcW, layout.srcH);

		drawDebugRegionForLayout(
			*hints,
			layout.dest,
			layout.srcX, layout.srcY, layout.srcW, layout.srcH);
		drawNeighborOverlays(hints, layout.dest, layout.srcX, layout.srcY, layout.srcW, layout.srcH);
		return;
	}

	const WidthFitLayout layout = widthFitLayout(mediaW, mediaH, bounds);
	float drawSrcX = srcX + layout.srcX;
	float drawSrcY = srcY + layout.srcY;
	float drawSrcW = layout.srcW;
	float drawSrcH = layout.srcH;
	if (hints) {
		animateSrcRect(*hints, fullW, fullH, drawSrcX, drawSrcY, drawSrcW, drawSrcH);
	}
	image.drawSubsection(
		layout.dest.x, layout.dest.y, layout.dest.width, layout.dest.height,
		drawSrcX, drawSrcY, drawSrcW, drawSrcH);

	if (hints) {
		drawDebugRegionForLayout(
			*hints,
			layout.dest,
			drawSrcX, drawSrcY, drawSrcW, drawSrcH);
		drawNeighborOverlays(hints, layout.dest, drawSrcX, drawSrcY, drawSrcW, drawSrcH);
	}
}
