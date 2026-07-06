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
#include <vector>

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
	void setAnimationsEnabled(bool enabled);
	bool animationsEnabled() const { return animationsEnabled_; }

	using ClipChangedHandler = std::function<void()>;
	void setClipChangedHandler(ClipChangedHandler handler);

private:
	void syncLoadedPath();
	void refreshImageDrawHints(const ofRectangle& bounds);
	void onClipSwitched(const MediaPlaybackEngine::SwitchResult& result);
	void pickAnimationForSelection();
	void refreshNeighborOverlays();

	ClipChangedHandler clipChangedHandler;

	MediaClipLibrary library;
	MediaCorpusProvider corpus_;
	MediaPlaybackEngine engine;
	ImageDrawHints imageDrawHints_;
	std::string loadedPath;
	mutable ofRectangle lastDrawBounds_;

	// Previous/next-2 thumbnails overlaid into blank spots on the current
	// image (see refreshNeighborOverlays()). Loaded only on an actual clip
	// switch; imageDrawHints_.neighbor_overlays is refreshed from
	// neighborOverlayAssignments_ on every refreshImageDrawHints() call
	// (including resize) without reloading anything.
	struct NeighborOverlayAssignment {
		metaagent::media::IntRect rect;
		const ofImage* thumb = nullptr;
		float rotationDeg = 0.0f;
	};
	ofImage prevThumb1_;
	ofImage prevThumb2_;
	ofImage nextThumb1_;
	ofImage nextThumb2_;
	std::vector<NeighborOverlayAssignment> neighborOverlayAssignments_;
	std::size_t selectedRegionIndex_ = std::numeric_limits<std::size_t>::max();
	bool showRegionBBox_ = true;
	// Region pan is the default display mode; zoom is the opt-in alternative.
	bool regionFocusEnabled_ = false;
	bool regionPanEnabled_ = true;
	// Selection animation: mode + motion parameters randomized per clip (see
	// ImageDrawHints and pickAnimationForSelection()).
	bool animationsEnabled_ = true;
	int selectedAnimMode_ = 0;
	float animStartSeconds_ = 0.0f;
	float animAmpX_ = 0.05f;
	float animAmpY_ = 0.05f;
	float animFreqX_ = 0.30f;
	float animFreqY_ = 0.22f;
	float animPhaseX_ = 0.0f;
	float animPhaseY_ = 0.0f;
	float animZoomAmp_ = 0.0f;
	float animZoomFreq_ = 0.08f;
	float animZoomPhase_ = 0.0f;
};
