#pragma once

#include "ofMain.h"
#include "ofVideoPlayer.h"

struct ImageDrawHints {
	bool has_focus_rect = false;
	bool cover_fit = false;
	float src_x = 0.0f;
	float src_y = 0.0f;
	float src_w = 0.0f;
	float src_h = 0.0f;

	/// Debug: detected region bbox in full-image pixel coordinates.
	bool has_debug_region = false;
	float debug_region_x = 0.0f;
	float debug_region_y = 0.0f;
	float debug_region_w = 0.0f;
	float debug_region_h = 0.0f;

	/// Width-fit scale, pan visible window to center on this point (no zoom).
	bool pan_to_region = false;
	float pan_center_x = 0.0f;
	float pan_center_y = 0.0f;
};

/// Draws media width-fit, vertically centered; tall content (e.g. A4) is center-cropped.
class MediaRenderer {
public:
	void draw(const ofVideoPlayer& player, const ofRectangle& bounds);
	void draw(const ofImage& image, const ofRectangle& bounds, const ImageDrawHints* hints = nullptr);
};
