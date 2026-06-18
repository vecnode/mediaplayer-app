/*
 * media-player-cpp — VideoRenderer
 * Copyright (c) vecnode 2026
 */

#include "VideoRenderer.h"

void VideoRenderer::draw(const ofVideoPlayer& player, const ofRectangle& bounds) {
	if (!player.isLoaded()) {
		return;
	}

	const ofTexture& tex = player.getTexture();
	if (tex.isAllocated()) {
		ofSetColor(255);
		tex.draw(bounds);
		return;
	}

	player.draw(bounds.x, bounds.y, bounds.width, bounds.height);
}

void VideoRenderer::draw(const ofImage& image, const ofRectangle& bounds) {
	if (!image.isAllocated()) {
		return;
	}

	ofSetColor(255);
	image.draw(bounds);
}
