/*
 * media-player-cpp — VideoPlaybackEngine
 * Copyright (c) vecnode 2026
 */

#include "VideoPlaybackEngine.h"
#include "PlatformVideo.h"

void VideoPlaybackEngine::setup() {
	ensureSlotConfigured(activeSlotIndex);
}

void VideoPlaybackEngine::attachClipSource(const IClipSource* source) {
	clipSource = source;
}

void VideoPlaybackEngine::setSwitchHandler(SwitchHandler handler) {
	switchHandler = std::move(handler);
}

void VideoPlaybackEngine::notifySwitch(const SwitchResult& result) {
	if (switchHandler) {
		switchHandler(result);
	}
}

void VideoPlaybackEngine::markActiveLoadGrace() {
	activeLoadGraceFrames = kActiveLoadGraceFrames;
}

void VideoPlaybackEngine::ensureSlotConfigured(int slotIndex) {
	if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= kSlotCount) {
		return;
	}
	if (!slotsConfigured[slotIndex]) {
		PlatformVideo::configurePlayer(slots[slotIndex]);
		slotsConfigured[slotIndex] = true;
	}
}

void VideoPlaybackEngine::invalidatePreload() {
	prefetchLoadState = PrefetchLoadState::Idle;
	prefetchTargetIndex = kInvalidClipIndex;
	prefetchLoadAttempts = 0;
}

bool VideoPlaybackEngine::isStandbyReadyFor(std::size_t expectedIndex) const {
	if (slotsClipIndex[standbySlotIndex()] != expectedIndex) {
		return false;
	}

	const auto& standby = standbyPlayer();
	return standby.isLoaded()
		&& standby.getWidth() > 0
		&& standby.getHeight() > 0
		&& prefetchLoadState != PrefetchLoadState::Loading;
}

void VideoPlaybackEngine::silenceStandby() {
	if (!standbyPlayer().isLoaded()) {
		return;
	}

	standbyPlayer().setVolume(0.0f);
	standbyPlayer().setPaused(true);
}

bool VideoPlaybackEngine::loadClipIntoSlotSync(int slotIndex, std::size_t clipIndex, bool logLoad) {
	if (!clipSource || clipIndex >= clipSource->size()) {
		return false;
	}

	ensureSlotConfigured(slotIndex);

	const VideoClip& clip = clipSource->clipAt(clipIndex);
	of::filesystem::path absPath = clip.absolutePath;
	absPath.make_preferred();

	if (!slots[slotIndex].load(PlatformVideo::loadPath(absPath))) {
		ofLogWarning("VideoPlaybackEngine") << "Failed to load " << clip.absolutePath;
		slotsClipIndex[slotIndex] = kInvalidClipIndex;
		return false;
	}

	slots[slotIndex].setLoopState(OF_LOOP_NORMAL);
	slots[slotIndex].setVolume(1.0f);
	slotsClipIndex[slotIndex] = clipIndex;

	if (logLoad) {
		ofLogNotice("VideoPlaybackEngine") << "Loaded " << clip.displayName
			<< " (" << slots[slotIndex].getWidth() << "x" << slots[slotIndex].getHeight() << ")";
	}

	return true;
}

void VideoPlaybackEngine::beginAsyncLoadIntoSlot(int slotIndex, std::size_t clipIndex) {
	if (!clipSource || clipIndex >= clipSource->size()) {
		return;
	}

	ensureSlotConfigured(slotIndex);

	const VideoClip& clip = clipSource->clipAt(clipIndex);
	of::filesystem::path absPath = clip.absolutePath;
	absPath.make_preferred();

	prefetchLoadState = PrefetchLoadState::Loading;
	prefetchTargetIndex = clipIndex;
	prefetchLoadAttempts = 0;
	slotsClipIndex[slotIndex] = kInvalidClipIndex;

	slots[slotIndex].loadAsync(PlatformVideo::loadPath(absPath));

	ofLogVerbose("VideoPlaybackEngine") << "Async prefetch started: " << clip.displayName;
}

void VideoPlaybackEngine::tickAsyncPrefetch() {
	if (prefetchLoadState != PrefetchLoadState::Loading || !clipSource) {
		return;
	}

	const int standbySlot = standbySlotIndex();
	standbyPlayer().update();
	++prefetchLoadAttempts;

	if (!standbyPlayer().isLoaded()
		|| standbyPlayer().getWidth() <= 0
		|| standbyPlayer().getHeight() <= 0) {

		if (prefetchLoadAttempts >= kAsyncPrefetchTimeoutFrames) {
			ofLogWarning("VideoPlaybackEngine") << "Async prefetch timed out for index "
				<< prefetchTargetIndex;
			invalidatePreload();
			preloadRetryCooldown = kPreloadRetryCooldownFrames;
		}
		return;
	}

	standbyPlayer().setLoopState(OF_LOOP_NORMAL);
	standbyPlayer().setVolume(1.0f);
	slotsClipIndex[standbySlot] = prefetchTargetIndex;
	silenceStandby();
	prefetchLoadState = PrefetchLoadState::Idle;
	prefetchLoadAttempts = 0;

	ofLogVerbose("VideoPlaybackEngine") << "Async prefetch ready: "
		<< clipSource->clipAt(prefetchTargetIndex).displayName;
}

void VideoPlaybackEngine::schedulePrefetch() {
	if (!clipSource || clipSource->size() < 2 || !activePlayer().isLoaded()) {
		return;
	}

	if (preloadRetryCooldown > 0) {
		--preloadRetryCooldown;
		return;
	}

	if (activeLoadGraceFrames > 0) {
		--activeLoadGraceFrames;
		return;
	}

	const std::size_t nextIndex = clipSource->nextIndex(currentClipIndex);

	if (isStandbyReadyFor(nextIndex)) {
		return;
	}

	if (prefetchLoadState == PrefetchLoadState::Loading) {
		if (prefetchTargetIndex == nextIndex) {
			return;
		}
		invalidatePreload();
	}

	beginAsyncLoadIntoSlot(standbySlotIndex(), nextIndex);
}

bool VideoPlaybackEngine::openIndex(std::size_t index, bool primePreviewFrame, bool emitSwitchEvent) {
	if (!clipSource || index >= clipSource->size()) {
		return false;
	}

	invalidatePreload();

	if (!loadClipIntoSlotSync(activeSlotIndex, index, true)) {
		return false;
	}

	currentClipIndex = index;
	markActiveLoadGrace();

	if (primePreviewFrame) {
		PlatformVideo::primeFirstFrame(activePlayer());
	}

	if (emitSwitchEvent) {
		SwitchResult result;
		result.success = true;
		result.method = SwitchMethod::SyncLoad;
		result.index = index;
		notifySwitch(result);
	}

	return true;
}

void VideoPlaybackEngine::swapToStandbyClip(std::size_t clipIndex) {
	activeSlotIndex = standbySlotIndex();
	currentClipIndex = clipIndex;

	activePlayer().setVolume(1.0f);
	activePlayer().setLoopState(OF_LOOP_NORMAL);
	invalidatePreload();

	ofLogNotice("VideoPlaybackEngine") << "Switched to "
		<< clipSource->clipAt(clipIndex).displayName << " (prefetched)";
}

void VideoPlaybackEngine::applyTransportAfterSwitch(bool wasPlaying) {
	if (wasPlaying) {
		activePlayer().setPaused(false);
		activePlayer().play();
	} else {
		PlatformVideo::primeFirstFrame(activePlayer());
	}
}

VideoPlaybackEngine::SwitchResult VideoPlaybackEngine::skipToIndex(std::size_t targetIndex) {
	SwitchResult result;

	if (!clipSource || clipSource->empty() || targetIndex >= clipSource->size()) {
		return result;
	}

	const bool wasPlaying = activePlayer().isPlaying();

	if (isStandbyReadyFor(targetIndex)) {
		swapToStandbyClip(targetIndex);
		result.method = SwitchMethod::PrefetchSwap;
		result.success = true;
	} else if (!openIndex(targetIndex, false)) {
		return result;
	} else {
		result.method = SwitchMethod::SyncLoad;
		result.success = true;
	}

	result.index = targetIndex;
	applyTransportAfterSwitch(wasPlaying);
	notifySwitch(result);
	return result;
}

VideoPlaybackEngine::SwitchResult VideoPlaybackEngine::skipToNext() {
	if (!clipSource || clipSource->empty()) {
		return {};
	}
	return skipToIndex(clipSource->nextIndex(currentClipIndex));
}

VideoPlaybackEngine::SwitchResult VideoPlaybackEngine::skipToPrevious() {
	if (!clipSource || clipSource->empty()) {
		return {};
	}
	return skipToIndex(clipSource->previousIndex(currentClipIndex));
}

void VideoPlaybackEngine::play() {
	if (!activePlayer().isLoaded()) {
		return;
	}

	activePlayer().setPaused(false);
	activePlayer().play();
}

void VideoPlaybackEngine::stop() {
	PlatformVideo::stopPlayback(activePlayer());
}

bool VideoPlaybackEngine::isLoaded() const {
	return activePlayer().isLoaded();
}

bool VideoPlaybackEngine::isPlaying() const {
	return activePlayer().isPlaying();
}

const VideoClip& VideoPlaybackEngine::currentClip() const {
	static const VideoClip kEmpty;
	if (!clipSource) {
		return kEmpty;
	}
	return clipSource->clipAt(currentClipIndex);
}

void VideoPlaybackEngine::update() {
	++frameCounter;

	if (activePlayer().isLoaded()) {
		activePlayer().update();
	}

	if (prefetchLoadState == PrefetchLoadState::Loading) {
		tickAsyncPrefetch();
	} else if (standbyPlayer().isLoaded()) {
		const bool deferStandbyPump = activeLoadGraceFrames > 0
			|| (activePlayer().isPlaying() && (frameCounter % kStandbyPumpIntervalFrames) != 0);
		if (!deferStandbyPump) {
			standbyPlayer().update();
		}
	}

	schedulePrefetch();
}

void VideoPlaybackEngine::draw(const ofRectangle& bounds) const {
	renderer.draw(activePlayer(), bounds);
}
