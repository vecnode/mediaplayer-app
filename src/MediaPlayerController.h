#pragma once

#include "SubtitlesOverlay.h"
#include "VideoPanel.h"
#include <cstddef>
#include <string>
#include <vector>

/// Snapshot of playback + UI state (mirrors the ofxGui status label).
struct MediaPlayerStatus {
	bool loaded = false;
	bool playing = false;
	std::size_t clipIndex = 0;
	std::size_t clipCount = 0;
	std::string clipName;
	bool subtitlesEnabled = false;
};

/// One entry in the playlist exposed over HTTP.
struct MediaPlayerClipInfo {
	std::size_t index = 0;
	std::string name;
	std::string path;
};

/// Shared command layer for GUI, HTTP, and future control surfaces (OSC, etc.).
class MediaPlayerController {
public:
	void setup(VideoPanel* panel, SubtitlesOverlay* subtitles);

	void play();
	void stop();
	void nextClip();
	void previousClip();
	bool openClipAtIndex(std::size_t index);

	bool setSubtitlesEnabled(bool enabled);
	bool isSubtitlesEnabled() const;

	MediaPlayerStatus getStatus() const;
	std::vector<MediaPlayerClipInfo> getClips() const;

private:
	void syncSubtitleText();

	VideoPanel* videoPanel = nullptr;
	SubtitlesOverlay* subtitlesOverlay = nullptr;
};
