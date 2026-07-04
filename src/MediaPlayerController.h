#pragma once

#include "MediaPanel.h"
#include "SubtitlesOverlay.h"
#include <cstddef>
#include <string>
#include <vector>

/// Snapshot of playback + UI state (mirrors the ofxGui status label).
struct MediaPlayerStatus {
	bool loaded = false;
	bool playing = false;
	bool isImage = false;
	std::size_t clipIndex = 0;
	std::size_t clipCount = 0;
	std::string clipName;
	bool subtitlesEnabled = false;
	std::string subtitleText;
};

/// One entry in the playlist exposed over HTTP.
struct MediaPlayerClipInfo {
	std::size_t index = 0;
	std::string name;
	std::string path;
	std::string mediaType;
};

/// Shared command layer for GUI, HTTP, and future control surfaces (OSC, etc.).
class MediaPlayerController {
public:
	void setup(MediaPanel* panel, SubtitlesOverlay* subtitles);

	void play();
	void stop();
	void nextClip();
	void previousClip();
	void randomClip();
	bool openClipAtIndex(std::size_t index);

	bool setSubtitlesEnabled(bool enabled);
	bool setSubtitleText(const std::string& text);
	void clearSubtitleOverride();
	bool isSubtitlesEnabled() const;
	bool isCurrentClipImage() const;

	bool setShowRegionBBox(bool enabled);
	bool showRegionBBox() const;
	bool setRegionFocusEnabled(bool enabled);
	bool regionFocusEnabled() const;
	bool setRegionPanEnabled(bool enabled);
	bool regionPanEnabled() const;
	bool setAnimationsEnabled(bool enabled);
	bool animationsEnabled() const;

	MediaPlayerStatus getStatus() const;
	std::string getSubtitleText() const;
	std::vector<MediaPlayerClipInfo> getClips() const;

private:
	void syncSubtitleText();

	MediaPanel* mediaPanel = nullptr;
	SubtitlesOverlay* subtitlesOverlay = nullptr;
	std::string subtitleOverride_;
};
