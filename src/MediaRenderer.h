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

	/// Selection animation, chosen per clip: 0 = none, 1 = drift (pan only),
	/// 2 = slow zoom (pan + breathing zoom). Never settles once started - both
	/// modes keep moving on both axes for as long as the clip is shown.
	/// Amplitude/frequency/phase are randomized per clip (see
	/// MediaPanel::pickAnimationForSelection) so consecutive clips read as
	/// visually distinct: some pan mostly vertically ("go down, go up"), some
	/// mostly horizontally, some as a diagonal/mixed drift.
	int anim_mode = 0;
	float anim_start_seconds = 0.0f;
	float anim_amp_x = 0.05f;
	float anim_amp_y = 0.05f;
	float anim_freq_x = 0.30f;
	float anim_freq_y = 0.22f;
	float anim_phase_x = 0.0f;
	float anim_phase_y = 0.0f;
	float anim_zoom_amp = 0.0f;
	float anim_zoom_freq = 0.08f;
	float anim_zoom_phase = 0.0f;

	/// One neighbor clip's thumbnail overlaid into a blank/text-free spot on
	/// the current image (see MediaCorpusProvider::emptyAreaRects). Rect is in
	/// full-image pixel coordinates, so it tracks the current image's pan/zoom
	/// automatically - the overlay stays glued to the same spot as it animates.
	struct NeighborOverlaySlot {
		bool has_rect = false;
		float rect_x = 0.0f;
		float rect_y = 0.0f;
		float rect_w = 0.0f;
		float rect_h = 0.0f;
		const ofImage* thumb = nullptr; // non-owning
		// Small fixed per-slot tilt (degrees), chosen once when the slot is
		// assigned so the "photo print" look reads as scattered but stable -
		// never re-rolled or animated frame to frame.
		float rotation_deg = 0.0f;
	};
	static constexpr int kMaxNeighborOverlays = 4;
	NeighborOverlaySlot neighbor_overlays[kMaxNeighborOverlays];
};

enum SelectionAnimMode {
	kAnimNone = 0,
	kAnimDrift = 1,
	kAnimSlowZoom = 2,
	kAnimModeCount = 3
};

/// Draws media width-fit, vertically centered; tall content (e.g. A4) is center-cropped.
class MediaRenderer {
public:
	void draw(const ofVideoPlayer& player, const ofRectangle& bounds);
	void draw(const ofImage& image, const ofRectangle& bounds, const ImageDrawHints* hints = nullptr);
};
