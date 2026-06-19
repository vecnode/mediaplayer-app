#pragma once

#include "IClipSource.h"
#include "MediaClipLibrary.h"
#include "MediaCorpusProvider.h"
#include "MediaPlaybackEngine.h"
#include "MediaRenderer.h"
#include "ofMain.h"
#include <functional>
#include <limits>
#include <string>

/// Composes playlist source + playback engine (stable facade for ofApp / controller).
class MediaPanel {
public:
	void setup();

	void update();
	void draw(const ofRectangle& bounds) const;

	void play();
	void stop();
	void cycleNext();
	void cyclePrevious();
	void cycleRandom();
	bool openClipAtIndex(std::size_t index, bool primePreviewFrame = true);

	bool isLoaded() const { return engine.isLoaded(); }
	bool isPlaying() const { return engine.isPlaying(); }
	bool isCurrentClipImage() const { return engine.isCurrentClipImage(); }
	const std::string& getLoadedPath() const { return loadedPath; }
	const std::string& getSearchLog() const { return library.getSearchLog(); }
	std::size_t getClipCount() const { return library.size(); }
	std::size_t getCurrentIndex() const { return engine.currentIndex(); }

	IClipSource& getClipSource() { return library; }
	const IClipSource& getClipSource() const { return library; }
	MediaClipLibrary& getLibrary() { return library; }
	const MediaClipLibrary& getLibrary() const { return library; }
	const MediaCorpusProvider& getCorpus() const { return corpus_; }
	MediaCorpusProvider& getCorpus() { return corpus_; }
	MediaPlaybackEngine& getEngine() { return engine; }
	const MediaPlaybackEngine& getEngine() const { return engine; }

	void setShowRegionBBox(bool show);
	bool showRegionBBox() const { return showRegionBBox_; }
	void setRegionFocusEnabled(bool enabled);
	bool regionFocusEnabled() const { return regionFocusEnabled_; }
	void setRegionPanEnabled(bool enabled);
	bool regionPanEnabled() const { return regionPanEnabled_; }

	using ClipChangedHandler = std::function<void()>;
	void setClipChangedHandler(ClipChangedHandler handler);

private:
	void syncLoadedPath();
	void refreshImageDrawHints(const ofRectangle& bounds);
	void onClipSwitched(const MediaPlaybackEngine::SwitchResult& result);

	ClipChangedHandler clipChangedHandler;

	MediaClipLibrary library;
	MediaCorpusProvider corpus_;
	MediaPlaybackEngine engine;
	ImageDrawHints imageDrawHints_;
	std::string loadedPath;
	mutable ofRectangle lastDrawBounds_;
	std::size_t selectedRegionIndex_ = std::numeric_limits<std::size_t>::max();
	bool showRegionBBox_ = true;
	bool regionFocusEnabled_ = true;
	bool regionPanEnabled_ = false;
};
