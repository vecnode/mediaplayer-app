#pragma once

#include "ofMain.h"
#include "ofVideoPlayer.h"

/// Draws the active video scaled to bounds every frame.
///
/// The GPU texture must be redrawn each frame because openFrameworks clears the
/// back buffer before draw(); skipping draws when the decode frame is unchanged
/// would show black. Decode work is still avoided upstream when the frame is not new.
class VideoRenderer {
public:
	void draw(const ofVideoPlayer& player, const ofRectangle& bounds);
};
