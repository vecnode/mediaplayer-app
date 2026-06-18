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

void VideoPlaybackEngine::clearSlot(int slotIndex) {
	if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= kSlotCount) {
		return;
	}

	slots[slotIndex].close();
	imageSlots[slotIndex].clear();
	slotsIsImage[slotIndex] = false;
	slotsClipIndex[slotIndex] = kInvalidClipIndex;
}

bool VideoPlaybackEngine::isSlotImage(int slotIndex) const {
	if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= kSlotCount) {
		return false;
	}
	return slotsIsImage[slotIndex];
}

bool VideoPlaybackEngine::isSlotLoaded(int slotIndex) const {
	if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= kSlotCount) {
		return false;
	}

	if (slotsIsImage[slotIndex]) {
		return imageSlots[slotIndex].isAllocated()
			&& imageSlots[slotIndex].getWidth() > 0
			&& imageSlots[slotIndex].getHeight() > 0;
	}

	const auto& player = slots[slotIndex];
	return player.isLoaded() && player.getWidth() > 0 && player.getHeight() > 0;
}

void VideoPlaybackEngine::invalidatePreload() {
	prefetchLoadState = PrefetchLoadState::Idle;
	prefetchTargetIndex = kInvalidClipIndex;
	prefetchLoadAttempts = 0;
}

bool VideoPlaybackEngine::isStandbyReadyFor(std::size_t expectedIndex) const {
	const int standbySlot = standbySlotIndex();
	if (slotsClipIndex[standbySlot] != expectedIndex) {
		return false;
	}

	if (prefetchLoadState == PrefetchLoadState::Loading) {
		return false;
	}

	return isSlotLoaded(standbySlot);
}

void VideoPlaybackEngine::silenceStandby() {
	if (isSlotImage(standbySlotIndex()) || !standbyPlayer().isLoaded()) {
		return;
	}

	standbyPlayer().setVolume(0.0f);
	standbyPlayer().setPaused(true);
}

bool VideoPlaybackEngine::loadClipIntoSlotSync(int slotIndex, std::size_t clipIndex, bool logLoad) {
	if (!clipSource || clipIndex >= clipSource->size()) {
		return false;
	}

	clearSlot(slotIndex);

	const VideoClip& clip = clipSource->clipAt(clipIndex);
	of::filesystem::path absPath = clip.absolutePath;
	absPath.make_preferred();

	if (clip.mediaType == ClipMediaType::Image) {
		if (!imageSlots[slotIndex].load(absPath.string())) {
			ofLogWarning("VideoPlaybackEngine") << "Failed to load image " << clip.absolutePath;
			return false;
		}

		slotsIsImage[slotIndex] = true;
		slotsClipIndex[slotIndex] = clipIndex;

		if (logLoad) {
			ofLogNotice("VideoPlaybackEngine") << "Loaded image " << clip.displayName
				<< " (" << imageSlots[slotIndex].getWidth() << "x" << imageSlots[slotIndex].getHeight() << ")";
		}

		return true;
	}

	ensureSlotConfigured(slotIndex);

	if (!slots[slotIndex].load(PlatformVideo::loadPath(absPath))) {
		ofLogWarning("VideoPlaybackEngine") << "Failed to load " << clip.absolutePath;
		return false;
	}

	slots[slotIndex].setLoopState(OF_LOOP_NORMAL);
	slots[slotIndex].setVolume(1.0f);
	slotsIsImage[slotIndex] = false;
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

	const VideoClip& clip = clipSource->clipAt(clipIndex);
	if (clip.mediaType == ClipMediaType::Image) {
		loadClipIntoSlotSync(slotIndex, clipIndex, false);
		return;
	}

	ensureSlotConfigured(slotIndex);

	of::filesystem::path absPath = clip.absolutePath;
	absPath.make_preferred();

	clearSlot(slotIndex);
	prefetchLoadState = PrefetchLoadState::Loading;
	prefetchTargetIndex = clipIndex;
	prefetchLoadAttempts = 0;

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
	if (!clipSource || clipSource->size() < 2 || !isSlotLoaded(activeSlotIndex)) {
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

	if (primePreviewFrame && !isSlotImage(activeSlotIndex)) {
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

	if (!isSlotImage(activeSlotIndex)) {
		activePlayer().setVolume(1.0f);
		activePlayer().setLoopState(OF_LOOP_NORMAL);
	}
	invalidatePreload();

	ofLogNotice("VideoPlaybackEngine") << "Switched to "
		<< clipSource->clipAt(clipIndex).displayName << " (prefetched)";
}

void VideoPlaybackEngine::applyTransportAfterSwitch(bool wasPlaying) {
	if (isSlotImage(activeSlotIndex)) {
		return;
	}

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
	if (!isSlotLoaded(activeSlotIndex) || isSlotImage(activeSlotIndex)) {
		return;
	}

	activePlayer().setPaused(false);
	activePlayer().play();
}

void VideoPlaybackEngine::stop() {
	if (isSlotImage(activeSlotIndex)) {
		return;
	}

	PlatformVideo::stopPlayback(activePlayer());
}

bool VideoPlaybackEngine::isLoaded() const {
	return isSlotLoaded(activeSlotIndex);
}

bool VideoPlaybackEngine::isPlaying() const {
	if (isSlotImage(activeSlotIndex)) {
		return false;
	}

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

	if (!isSlotImage(activeSlotIndex) && activePlayer().isLoaded()) {
		activePlayer().update();
	}

	if (prefetchLoadState == PrefetchLoadState::Loading) {
		tickAsyncPrefetch();
	} else if (!isSlotImage(standbySlotIndex()) && standbyPlayer().isLoaded()) {
		const bool deferStandbyPump = activeLoadGraceFrames > 0
			|| (activePlayer().isPlaying() && (frameCounter % kStandbyPumpIntervalFrames) != 0);
		if (!deferStandbyPump) {
			standbyPlayer().update();
		}
	}

	schedulePrefetch();
}

void VideoPlaybackEngine::draw(const ofRectangle& bounds) const {
	if (isSlotImage(activeSlotIndex)) {
		renderer.draw(imageSlots[activeSlotIndex], bounds);
		return;
	}

	renderer.draw(activePlayer(), bounds);
}
