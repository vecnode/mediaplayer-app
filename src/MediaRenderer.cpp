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

/// A "cross" world: current sits in the center panel, one neighbor touches
/// each of its four edges (top/left/right/bottom), all panels the same size
/// as `bounds`. The camera (viewport) is also `bounds`-sized and pans/zooms
/// through this bigger virtual canvas via the exact same animation math as
/// the single-image case, so it can drift off the current panel and reveal
/// a neighbor at the screen edge - no gaps, no black borders, since every
/// panel is cover-fit (crop-to-fill, never letterboxed) and panels are
/// contiguous.
struct CrossPanel {
	const ofImage* image = nullptr;
	ofRectangle worldRect;
};

struct CrossWorld {
	float worldW = 0.0f;
	float worldH = 0.0f;
	CrossPanel panels[5];
};

CrossWorld computeCrossWorld(const ofImage& current, const ImageDrawHints& hints, const ofRectangle& bounds) {
	CrossWorld world;
	const float panelW = bounds.width;
	const float panelH = bounds.height;
	world.worldW = panelW * 3.0f;
	world.worldH = panelH * 3.0f;

	world.panels[0] = {&current, {panelW, panelH, panelW, panelH}};              // center
	world.panels[1] = {hints.neighbor_prev1, {panelW, 0.0f, panelW, panelH}};     // top
	world.panels[2] = {hints.neighbor_prev2, {0.0f, panelH, panelW, panelH}};     // left
	world.panels[3] = {hints.neighbor_next1, {panelW * 2.0f, panelH, panelW, panelH}}; // right
	world.panels[4] = {hints.neighbor_next2, {panelW, panelH * 2.0f, panelW, panelH}}; // bottom
	return world;
}

/// Cover-fit scale+offset that places `imgW x imgH` into `panelRect` with no
/// letterboxing (crops overflow instead).
void coverFitTransform(
	float imgW, float imgH, const ofRectangle& panelRect,
	float& scale, float& offsetX, float& offsetY) {
	scale = std::max(panelRect.width / imgW, panelRect.height / imgH);
	offsetX = (panelRect.width - imgW * scale) * 0.5f;
	offsetY = (panelRect.height - imgH * scale) * 0.5f;
}

/// Draws whatever part of `panel` falls within `viewport` (both in world
/// coordinates), mapped onto the matching part of `destBounds` on screen.
void drawCrossPanelIntersection(const CrossPanel& panel, const ofRectangle& viewport, const ofRectangle& destBounds) {
	if (!panel.image || !panel.image->isAllocated()) {
		return;
	}

	const float ix = std::max(panel.worldRect.x, viewport.x);
	const float iy = std::max(panel.worldRect.y, viewport.y);
	const float ix2 = std::min(panel.worldRect.x + panel.worldRect.width, viewport.x + viewport.width);
	const float iy2 = std::min(panel.worldRect.y + panel.worldRect.height, viewport.y + viewport.height);
	const float iw = ix2 - ix;
	const float ih = iy2 - iy;
	if (iw <= 0.0f || ih <= 0.0f) {
		return;
	}

	const float imgW = static_cast<float>(panel.image->getWidth());
	const float imgH = static_cast<float>(panel.image->getHeight());
	if (imgW <= 0.0f || imgH <= 0.0f) {
		return;
	}

	float scale = 1.0f, offsetX = 0.0f, offsetY = 0.0f;
	coverFitTransform(imgW, imgH, panel.worldRect, scale, offsetX, offsetY);

	const float srcX = (ix - panel.worldRect.x - offsetX) / scale;
	const float srcY = (iy - panel.worldRect.y - offsetY) / scale;
	const float srcW = iw / scale;
	const float srcH = ih / scale;

	const ofRectangle screenRect = mapImageRectToScreen(
		destBounds, viewport.x, viewport.y, viewport.width, viewport.height, ix, iy, iw, ih);

	ofSetColor(255);
	panel.image->drawSubsection(screenRect.x, screenRect.y, screenRect.width, screenRect.height, srcX, srcY, srcW, srcH);
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

/// Cross-canvas path: current + neighbors cover-fit into 5 equal, touching
/// panels; the camera pans/zooms through this bigger canvas (reusing the
/// current clip's own focus/pan target, converted into world coordinates, as
/// the starting point) so it can drift off current and reveal a neighbor.
void drawCrossCanvas(const ofImage& image, const ofRectangle& bounds, const ImageDrawHints& hints) {
	const CrossWorld world = computeCrossWorld(image, hints, bounds);
	const float panelW = bounds.width;
	const float panelH = bounds.height;

	ofRectangle viewport = {panelW, panelH, panelW, panelH};

	const float imgW = static_cast<float>(image.getWidth());
	const float imgH = static_cast<float>(image.getHeight());
	if (imgW > 0.0f && imgH > 0.0f) {
		float scale = 1.0f, offsetX = 0.0f, offsetY = 0.0f;
		coverFitTransform(imgW, imgH, world.panels[0].worldRect, scale, offsetX, offsetY);

		float targetImgX = imgW * 0.5f;
		float targetImgY = imgH * 0.5f;
		if (hints.pan_to_region) {
			targetImgX = hints.pan_center_x;
			targetImgY = hints.pan_center_y;
		} else if (hints.has_focus_rect) {
			targetImgX = hints.src_x + hints.src_w * 0.5f;
			targetImgY = hints.src_y + hints.src_h * 0.5f;
		}

		const float targetWorldX = panelW + offsetX + targetImgX * scale;
		const float targetWorldY = panelH + offsetY + targetImgY * scale;
		viewport.x = targetWorldX - panelW * 0.5f;
		viewport.y = targetWorldY - panelH * 0.5f;
	}

	animateSrcRect(hints, world.worldW, world.worldH, viewport.x, viewport.y, viewport.width, viewport.height);

	// Never let the camera drift diagonally toward a corner - each neighbor
	// only touches current on ONE side, so a corner has no image at all.
	// Locking whichever axis has the smaller offset back to center keeps the
	// camera exactly on the horizontal or vertical arm of the cross.
	const float offsetFromCenterX = viewport.x - panelW;
	const float offsetFromCenterY = viewport.y - panelH;
	if (std::abs(offsetFromCenterX) > std::abs(offsetFromCenterY)) {
		viewport.y = panelH - (viewport.height - panelH) * 0.5f;
	} else {
		viewport.x = panelW - (viewport.width - panelW) * 0.5f;
	}

	for (const CrossPanel& panel : world.panels) {
		drawCrossPanelIntersection(panel, viewport, bounds);
	}

	if (hints.has_debug_region) {
		float scale = 1.0f, offsetX = 0.0f, offsetY = 0.0f;
		coverFitTransform(imgW, imgH, world.panels[0].worldRect, scale, offsetX, offsetY);
		const float worldX = panelW + offsetX + hints.debug_region_x * scale;
		const float worldY = panelH + offsetY + hints.debug_region_y * scale;
		const float worldW = hints.debug_region_w * scale;
		const float worldH = hints.debug_region_h * scale;
		const ofRectangle screenRect = mapImageRectToScreen(
			bounds, viewport.x, viewport.y, viewport.width, viewport.height, worldX, worldY, worldW, worldH);
		drawDebugRegionOutline(screenRect);
	}
}

void MediaRenderer::draw(const ofImage& image, const ofRectangle& bounds, const ImageDrawHints* hints) {
	if (!image.isAllocated()) {
		return;
	}

	const bool hasNeighbors = hints
		&& (hints->neighbor_prev2 || hints->neighbor_prev1 || hints->neighbor_next1 || hints->neighbor_next2);
	if (hasNeighbors) {
		drawCrossCanvas(image, bounds, *hints);
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
	}
}
