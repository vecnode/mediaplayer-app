/*
 * media-player-cpp — MediaRenderer
 * Copyright (c) vecnode 2026
 */

#include "MediaRenderer.h"

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

void MediaRenderer::draw(const ofImage& image, const ofRectangle& bounds) {
	if (!image.isAllocated()) {
		return;
	}

	drawWidthFitMedia(bounds,
		static_cast<float>(image.getWidth()),
		static_cast<float>(image.getHeight()),
		[&](const WidthFitLayout& layout) {
			image.drawSubsection(
				layout.dest.x, layout.dest.y, layout.dest.width, layout.dest.height,
				layout.srcX, layout.srcY, layout.srcW, layout.srcH);
		});
}
