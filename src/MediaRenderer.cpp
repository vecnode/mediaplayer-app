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

WidthFitLayout coverFitLayout(float mediaW, float mediaH, const ofRectangle& bounds) {
	WidthFitLayout layout;

	if (mediaW <= 0.0f || mediaH <= 0.0f || bounds.width <= 0.0f || bounds.height <= 0.0f) {
		layout.dest = bounds;
		layout.srcW = mediaW;
		layout.srcH = mediaH;
		return layout;
	}

	const float scale = std::max(bounds.width / mediaW, bounds.height / mediaH);
	layout.dest.width = mediaW * scale;
	layout.dest.height = mediaH * scale;
	layout.dest.x = bounds.x + (bounds.width - layout.dest.width) * 0.5f;
	layout.dest.y = bounds.y + (bounds.height - layout.dest.height) * 0.5f;
	layout.srcX = 0.0f;
	layout.srcY = 0.0f;
	layout.srcW = mediaW;
	layout.srcH = mediaH;
	return layout;
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

	float mediaW = static_cast<float>(image.getWidth());
	float mediaH = static_cast<float>(image.getHeight());
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

	const WidthFitLayout layout = (use_focus && hints->cover_fit)
		? coverFitLayout(mediaW, mediaH, bounds)
		: widthFitLayout(mediaW, mediaH, bounds);

	image.drawSubsection(
		layout.dest.x, layout.dest.y, layout.dest.width, layout.dest.height,
		srcX + layout.srcX, srcY + layout.srcY, layout.srcW, layout.srcH);
}
