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

void drawWidthFitMedia(const ofRectangle& bounds, float mediaW, float mediaH,
	const std::function<void(const WidthFitLayout&)>& drawMedia) {
	ofSetColor(0);
	ofDrawRectangle(bounds);

	const WidthFitLayout layout = widthFitLayout(mediaW, mediaH, bounds);
	ofSetColor(255);
	drawMedia(layout);
}

/// Applies the selection animation to a source crop window, keeping it inside
/// the full image. Both modes pan continuously on both axes and never settle -
/// there is no "resting" state once animation starts. Drift wanders the window
/// on a slow Lissajous path (no zoom); slow zoom does the same continuous pan
/// plus a never-settling breathing zoom (a Ken Burns effect that keeps moving
/// indefinitely instead of easing to a fixed end state).
void animateSrcRect(const ImageDrawHints& hints,
	float fullW, float fullH,
	float& srcX, float& srcY, float& srcW, float& srcH) {
	if (hints.anim_mode == kAnimNone || srcW <= 0.0f || srcH <= 0.0f) {
		return;
	}

	const float t = ofGetElapsedTimef() - hints.anim_start_seconds;

	if (hints.anim_mode == kAnimDrift) {
		srcX += 0.03f * srcW * std::sin(t * 0.35f);
		srcY += 0.03f * srcH * std::cos(t * 0.27f);
	} else if (hints.anim_mode == kAnimSlowZoom) {
		// Zoom oscillates between 90% and 100% forever (~78s period) instead
		// of easing to a fixed 90% and stopping - it never stops moving.
		const float zoomWave = 0.5f + 0.5f * std::sin(t * 0.08f);
		const float factor = 1.0f - 0.10f * zoomWave;
		const float centerX = srcX + srcW * 0.5f + 0.04f * srcW * std::sin(t * 0.22f);
		const float centerY = srcY + srcH * 0.5f + 0.04f * srcH * std::cos(t * 0.17f);
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
