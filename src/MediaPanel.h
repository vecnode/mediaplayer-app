#pragma once

#include "IClipSource.h"
#include "MediaClipLibrary.h"
#include "MediaPlaybackEngine.h"
#include "ofMain.h"
#include <functional>
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
	MediaPlaybackEngine& getEngine() { return engine; }
	const MediaPlaybackEngine& getEngine() const { return engine; }

	using ClipChangedHandler = std::function<void()>;
	void setClipChangedHandler(ClipChangedHandler handler);

private:
	void syncLoadedPath();
	void onClipSwitched(const MediaPlaybackEngine::SwitchResult& result);

	ClipChangedHandler clipChangedHandler;

	MediaClipLibrary library;
	MediaPlaybackEngine engine;
	std::string loadedPath;
};
