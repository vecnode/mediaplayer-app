/*
 * media-player-cpp — MediaRenderer
 * Copyright (c) vecnode 2026
 */

#include "MediaRenderer.h"

#include <algorithm>
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
		image.drawSubsection(
			bounds.x, bounds.y, bounds.width, bounds.height,
			srcX, srcY, srcW, srcH);

		if (hints) {
			drawDebugRegionForLayout(*hints, bounds, srcX, srcY, srcW, srcH);
		}
		return;
	}

	const WidthFitLayout layout = widthFitLayout(mediaW, mediaH, bounds);
	image.drawSubsection(
		layout.dest.x, layout.dest.y, layout.dest.width, layout.dest.height,
		srcX + layout.srcX, srcY + layout.srcY, layout.srcW, layout.srcH);

	if (hints) {
		drawDebugRegionForLayout(
			*hints,
			layout.dest,
			srcX + layout.srcX, srcY + layout.srcY, layout.srcW, layout.srcH);
	}
}
