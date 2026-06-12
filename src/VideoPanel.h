#pragma once

#include "VideoClipLibrary.h"
#include "VideoPlaybackEngine.h"
#include "ofMain.h"

/// Application-facing video panel: composes playlist discovery + playback engine.
///
/// Keeps a stable, minimal API for ofApp while playback internals remain extensible
/// via VideoClipLibrary (source) and VideoPlaybackEngine (transport/render).
class VideoPanel {
public:
	void setup();

	void update();
	void draw(const ofRectangle& bounds) const;

	void play();
	void stop();
	void cycleNext();
	bool openClipAtIndex(std::size_t index, bool primePreviewFrame = true);

	bool isLoaded() const { return engine.isLoaded(); }
	bool isPlaying() const { return engine.isPlaying(); }
	const std::string& getLoadedPath() const { return loadedPath; }
	const std::string& getSearchLog() const { return library.getSearchLog(); }
	std::size_t getClipCount() const { return library.size(); }
	std::size_t getCurrentIndex() const { return engine.currentIndex(); }

	/// Direct access for future features (subtitles sync, OSC, etc.).
	VideoClipLibrary& getLibrary() { return library; }
	const VideoClipLibrary& getLibrary() const { return library; }
	VideoPlaybackEngine& getEngine() { return engine; }
	const VideoPlaybackEngine& getEngine() const { return engine; }

private:
	void syncLoadedPath();

	VideoClipLibrary library;
	VideoPlaybackEngine engine;
	std::string loadedPath;
};
