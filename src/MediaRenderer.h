#pragma once

#include "ofMain.h"
#include "ofVideoPlayer.h"

/// Draws media width-fit, vertically centered; tall content (e.g. A4) is center-cropped.
class MediaRenderer {
public:
	void draw(const ofVideoPlayer& player, const ofRectangle& bounds);
	void draw(const ofImage& image, const ofRectangle& bounds);
};
