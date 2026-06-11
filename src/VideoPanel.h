#pragma once

#include "ofMain.h"
#include <string>
#include <vector>

/// Full-screen video panel backed by ofVideoPlayer.
///
/// Discovers clips next to the executable ({exe}/data, then {exe}/),
/// loads the first match on setup, and renders via the GPU texture path.
class VideoPanel {
public:
	void setup();

	void update();
	void draw(const ofRectangle& bounds) const;

	void play();
	void stop();
	void cycleNext();

	bool isLoaded() const { return player.isLoaded(); }
	bool isPlaying() const { return player.isPlaying(); }
	const std::string& getLoadedPath() const { return loadedPath; }
	const std::string& getSearchLog() const { return searchLog; }
	std::size_t getClipCount() const { return videoPaths.size(); }
	std::size_t getCurrentIndex() const { return currentIndex; }

private:
	void scanDataFolder();
	bool loadAtIndex(std::size_t index);
	void primeFirstFrame();

	ofVideoPlayer player;
	std::vector<std::string> videoPaths; ///< Absolute paths discovered on disk.
	std::size_t currentIndex = 0;
	std::string loadedPath;
	std::string searchLog;
};
