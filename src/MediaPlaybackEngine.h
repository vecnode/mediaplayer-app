#pragma once

#include "IClipSource.h"
#include "MediaRenderer.h"
#include "ofMain.h"
#include "ofVideoPlayer.h"

#include <atomic>
#include <cstddef>
#include <functional>
#include <limits>
#include <thread>

/// Dual-buffer playback engine for images and video.
///
/// Images load synchronously; videos use async standby prefetch when possible.
class MediaPlaybackEngine {
public:
	enum class SwitchMethod {
		None,
		PrefetchSwap,
		SyncLoad
	};

	struct SwitchResult {
		bool success = false;
		SwitchMethod method = SwitchMethod::None;
		std::size_t index = 0;
	};

	using SwitchHandler = std::function<void(const SwitchResult&)>;

	MediaPlaybackEngine() = default;
	~MediaPlaybackEngine();
	MediaPlaybackEngine(const MediaPlaybackEngine&) = delete;
	MediaPlaybackEngine& operator=(const MediaPlaybackEngine&) = delete;

	void setup();
	void attachClipSource(const IClipSource* source);
	void setSwitchHandler(SwitchHandler handler);
	void setImageDrawHints(const ImageDrawHints* hints);

	void update();
	void draw(const ofRectangle& bounds) const;

	bool openIndex(std::size_t index, bool primePreviewFrame = true, bool emitSwitchEvent = false);
	SwitchResult skipToNext();
	SwitchResult skipToPrevious();
	SwitchResult skipToRandom();

	void play();
	void stop();

	bool isLoaded() const;
	bool isPlaying() const;
	bool isCurrentClipImage() const;
	std::size_t currentIndex() const { return currentClipIndex; }
	const MediaClip& currentClip() const;

private:
	enum class PrefetchLoadState {
		Idle,
		Loading
	};

	static constexpr std::size_t kSlotCount = 2;
	static constexpr std::size_t kInvalidClipIndex = std::numeric_limits<std::size_t>::max();
	static constexpr int kPreloadRetryCooldownFrames = 120;
	static constexpr int kActiveLoadGraceFrames = 45;
	static constexpr int kStandbyPumpIntervalFrames = 2;
	static constexpr int kAsyncPrefetchTimeoutFrames = 600;

	void ensureSlotConfigured(int slotIndex);
	void clearSlot(int slotIndex);
	bool isSlotImage(int slotIndex) const;
	bool isSlotLoaded(int slotIndex) const;
	bool loadClipIntoSlotSync(int slotIndex, std::size_t clipIndex, bool logLoad);
	void beginAsyncLoadIntoSlot(int slotIndex, std::size_t clipIndex);
	void beginAsyncImagePrefetch(int slotIndex, std::size_t clipIndex);
	void tickAsyncPrefetch();
	void tickAsyncImagePrefetch();
	void schedulePrefetch();
	void invalidatePreload();
	void joinImagePrefetchThread();
	bool isStandbyReadyFor(std::size_t expectedIndex) const;
	void silenceStandby();
	SwitchResult skipToIndex(std::size_t targetIndex);
	void swapToStandbyClip(std::size_t clipIndex);
	void applyTransportAfterSwitch(bool wasPlaying);
	void notifySwitch(const SwitchResult& result);
	void markActiveLoadGrace();

	int standbySlotIndex() const { return 1 - activeSlotIndex; }
	ofVideoPlayer& activePlayer() { return slots[activeSlotIndex]; }
	const ofVideoPlayer& activePlayer() const { return slots[activeSlotIndex]; }
	ofVideoPlayer& standbyPlayer() { return slots[standbySlotIndex()]; }
	const ofVideoPlayer& standbyPlayer() const { return slots[standbySlotIndex()]; }

	const IClipSource* clipSource = nullptr;
	SwitchHandler switchHandler;
	const ImageDrawHints* imageDrawHints = nullptr;

	mutable MediaRenderer renderer;

	ofVideoPlayer slots[kSlotCount];
	ofImage imageSlots[kSlotCount];
	bool slotsIsImage[kSlotCount] = {false, false};
	bool slotsConfigured[kSlotCount] = {false, false};
	std::size_t slotsClipIndex[kSlotCount] = {kInvalidClipIndex, kInvalidClipIndex};

	int activeSlotIndex = 0;
	std::size_t currentClipIndex = 0;

	PrefetchLoadState prefetchLoadState = PrefetchLoadState::Idle;
	std::size_t prefetchTargetIndex = kInvalidClipIndex;
	bool prefetchIsImage = false;
	int preloadRetryCooldown = 0;
	int activeLoadGraceFrames = 0;
	int prefetchLoadAttempts = 0;
	int frameCounter = 0;

	// Background image decode: worker thread decodes pixels (no GL calls, safe
	// off-thread - same approach openFrameworks' own ofxThreadedImageLoader
	// uses); the main thread uploads the finished pixels to a GPU texture in
	// tickAsyncImagePrefetch(). imagePrefetchDone gates cross-thread visibility
	// of imagePrefetchPixels/imagePrefetchSuccess (release in the worker,
	// acquire on the main thread via the atomic load).
	std::thread imagePrefetchThread;
	std::atomic<bool> imagePrefetchDone{false};
	bool imagePrefetchSuccess = false;
	ofPixels imagePrefetchPixels;
	int imagePrefetchSlot = -1;
	std::size_t imagePrefetchClipIndex = kInvalidClipIndex;
};
