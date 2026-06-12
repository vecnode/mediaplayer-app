#pragma once

#include "ofMain.h"
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

/// Full-screen video panel with dual-player prefetch for fast clip changes.
///
/// Architecture (ping-pong):
///   - players[0] and players[1] each own a platform video backend.
///   - activeSlot points at the visible/audible player.
///   - The other slot silently preloads (currentIndex + 1) % clipCount.
///   - cycleNext() swaps slots when prefetch is ready; otherwise falls back to sync load.
///
/// Clip discovery searches {exe}/data, then {exe}/ (and bin/ paths in debug builds).
class VideoPanel {
public:
	void setup();

	void update();
	void draw(const ofRectangle& bounds) const;

	void play();
	void stop();
	void cycleNext();

	bool isLoaded() const { return activePlayer().isLoaded(); }
	bool isPlaying() const { return activePlayer().isPlaying(); }
	const std::string& getLoadedPath() const { return loadedPath; }
	const std::string& getSearchLog() const { return searchLog; }
	std::size_t getClipCount() const { return videoPaths.size(); }
	std::size_t getCurrentIndex() const { return currentIndex; }

private:
	static constexpr std::size_t kInvalidPreloadIndex = std::numeric_limits<std::size_t>::max();
	static constexpr int kPreloadRetryCooldownFrames = 120; ///< Wait before retrying a failed prefetch.

	void scanDataFolder();
	void ensurePlayerConfigured(ofVideoPlayer& player, bool& configuredFlag);
	bool loadIntoPlayer(ofVideoPlayer& player, std::size_t index, bool logLoad);
	bool loadAtIndex(std::size_t index);
	void preloadNextClip();
	void invalidatePreload();
	bool isPreloadReady(std::size_t expectedIndex) const;
	void silenceStandby();
	void swapToPrefetchedClip(std::size_t nextIndex);
	void applyPlaybackStateAfterSwitch(bool wasPlaying);

	int standbySlot() const { return 1 - activeSlot; }
	ofVideoPlayer& activePlayer() { return players[activeSlot]; }
	const ofVideoPlayer& activePlayer() const { return players[activeSlot]; }
	ofVideoPlayer& standbyPlayer() { return players[standbySlot()]; }
	const ofVideoPlayer& standbyPlayer() const { return players[standbySlot()]; }

	ofVideoPlayer players[2];
	int activeSlot = 0;
	bool playersConfigured[2] = {false, false};

	std::vector<std::string> videoPaths;
	std::size_t currentIndex = 0;
	std::size_t preloadedIndex = kInvalidPreloadIndex;
	int preloadRetryCooldown = 0;

	std::string loadedPath;
	std::string searchLog;
};
