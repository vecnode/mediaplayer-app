/*
 * media-player-cpp — MediaPlaybackEngine
 * Copyright (c) vecnode 2026
 */

#include "MediaPlaybackEngine.h"
#include "PlatformVideo.h"

void MediaPlaybackEngine::setup() {
	ensureSlotConfigured(activeSlotIndex);
}

void MediaPlaybackEngine::attachClipSource(const IClipSource* source) {
	clipSource = source;
}

void MediaPlaybackEngine::setSwitchHandler(SwitchHandler handler) {
	switchHandler = std::move(handler);
}

void MediaPlaybackEngine::notifySwitch(const SwitchResult& result) {
	if (switchHandler) {
		switchHandler(result);
	}
}

void MediaPlaybackEngine::markActiveLoadGrace() {
	activeLoadGraceFrames = kActiveLoadGraceFrames;
}

void MediaPlaybackEngine::ensureSlotConfigured(int slotIndex) {
	if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= kSlotCount) {
		return;
	}
	if (!slotsConfigured[slotIndex]) {
		PlatformVideo::configurePlayer(slots[slotIndex]);
		slotsConfigured[slotIndex] = true;
	}
}

void MediaPlaybackEngine::clearSlot(int slotIndex) {
	if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= kSlotCount) {
		return;
	}

	slots[slotIndex].close();
	imageSlots[slotIndex].clear();
	slotsIsImage[slotIndex] = false;
	slotsClipIndex[slotIndex] = kInvalidClipIndex;
}

bool MediaPlaybackEngine::isSlotImage(int slotIndex) const {
	if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= kSlotCount) {
		return false;
	}
	return slotsIsImage[slotIndex];
}

bool MediaPlaybackEngine::isSlotLoaded(int slotIndex) const {
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

void MediaPlaybackEngine::invalidatePreload() {
	prefetchLoadState = PrefetchLoadState::Idle;
	prefetchTargetIndex = kInvalidClipIndex;
	prefetchLoadAttempts = 0;
}

bool MediaPlaybackEngine::isStandbyReadyFor(std::size_t expectedIndex) const {
	const int standbySlot = standbySlotIndex();
	if (slotsClipIndex[standbySlot] != expectedIndex) {
		return false;
	}

	if (prefetchLoadState == PrefetchLoadState::Loading) {
		return false;
	}

	return isSlotLoaded(standbySlot);
}

void MediaPlaybackEngine::silenceStandby() {
	if (isSlotImage(standbySlotIndex()) || !standbyPlayer().isLoaded()) {
		return;
	}

	standbyPlayer().setVolume(0.0f);
	standbyPlayer().setPaused(true);
}

bool MediaPlaybackEngine::loadClipIntoSlotSync(int slotIndex, std::size_t clipIndex, bool logLoad) {
	if (!clipSource || clipIndex >= clipSource->size()) {
		return false;
	}

	clearSlot(slotIndex);

	const MediaClip& clip = clipSource->clipAt(clipIndex);
	of::filesystem::path absPath = clip.absolutePath;
	absPath.make_preferred();

	if (clip.mediaType == ClipMediaType::Image) {
		if (!imageSlots[slotIndex].load(absPath)) {
			ofLogWarning("MediaPlaybackEngine") << "Failed to load image " << clip.absolutePath;
			return false;
		}

		slotsIsImage[slotIndex] = true;
		slotsClipIndex[slotIndex] = clipIndex;

		if (logLoad) {
			ofLogNotice("MediaPlaybackEngine") << "Loaded image " << clip.displayName
				<< " (" << imageSlots[slotIndex].getWidth() << "x" << imageSlots[slotIndex].getHeight() << ")";
		}

		return true;
	}

	ensureSlotConfigured(slotIndex);

	if (!slots[slotIndex].load(PlatformVideo::loadPath(absPath))) {
		ofLogWarning("MediaPlaybackEngine") << "Failed to load " << clip.absolutePath;
		return false;
	}

	slots[slotIndex].setLoopState(OF_LOOP_NORMAL);
	slots[slotIndex].setVolume(1.0f);
	slotsIsImage[slotIndex] = false;
	slotsClipIndex[slotIndex] = clipIndex;

	if (logLoad) {
		ofLogNotice("MediaPlaybackEngine") << "Loaded " << clip.displayName
			<< " (" << slots[slotIndex].getWidth() << "x" << slots[slotIndex].getHeight() << ")";
	}

	return true;
}

void MediaPlaybackEngine::beginAsyncLoadIntoSlot(int slotIndex, std::size_t clipIndex) {
	if (!clipSource || clipIndex >= clipSource->size()) {
		return;
	}

	const MediaClip& clip = clipSource->clipAt(clipIndex);
	if (clip.mediaType == ClipMediaType::Image) {
		// Images are large — load only when the user switches clips.
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

	ofLogVerbose("MediaPlaybackEngine") << "Async prefetch started: " << clip.displayName;
}

void MediaPlaybackEngine::tickAsyncPrefetch() {
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
			ofLogWarning("MediaPlaybackEngine") << "Async prefetch timed out for index "
				<< prefetchTargetIndex;
			invalidatePreload();
			preloadRetryCooldown = kPreloadRetryCooldownFrames;
		}
		return;
	}

	standbyPlayer().setLoopState(OF_LOOP_NORMAL);
	standbyPlayer().setVolume(1.0f);
	slotsIsImage[standbySlot] = false;
	slotsClipIndex[standbySlot] = prefetchTargetIndex;
	silenceStandby();
	prefetchLoadState = PrefetchLoadState::Idle;
	prefetchLoadAttempts = 0;

	ofLogVerbose("MediaPlaybackEngine") << "Async prefetch ready: "
		<< clipSource->clipAt(prefetchTargetIndex).displayName;
}

void MediaPlaybackEngine::schedulePrefetch() {
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

	if (clipSource->clipAt(nextIndex).mediaType == ClipMediaType::Image) {
		return;
	}

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

bool MediaPlaybackEngine::openIndex(std::size_t index, bool primePreviewFrame, bool emitSwitchEvent) {
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

void MediaPlaybackEngine::swapToStandbyClip(std::size_t clipIndex) {
	activeSlotIndex = standbySlotIndex();
	currentClipIndex = clipIndex;

	if (!isSlotImage(activeSlotIndex)) {
		activePlayer().setVolume(1.0f);
		activePlayer().setLoopState(OF_LOOP_NORMAL);
	}
	invalidatePreload();

	ofLogNotice("MediaPlaybackEngine") << "Switched to "
		<< clipSource->clipAt(clipIndex).displayName << " (prefetched)";
}

void MediaPlaybackEngine::applyTransportAfterSwitch(bool wasPlaying) {
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

MediaPlaybackEngine::SwitchResult MediaPlaybackEngine::skipToIndex(std::size_t targetIndex) {
	SwitchResult result;

	if (!clipSource || clipSource->empty() || targetIndex >= clipSource->size()) {
		return result;
	}

	const bool wasPlaying = !isSlotImage(activeSlotIndex) && activePlayer().isPlaying();

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

MediaPlaybackEngine::SwitchResult MediaPlaybackEngine::skipToNext() {
	if (!clipSource || clipSource->empty()) {
		return {};
	}
	return skipToIndex(clipSource->nextIndex(currentClipIndex));
}

MediaPlaybackEngine::SwitchResult MediaPlaybackEngine::skipToPrevious() {
	if (!clipSource || clipSource->empty()) {
		return {};
	}
	return skipToIndex(clipSource->previousIndex(currentClipIndex));
}

void MediaPlaybackEngine::play() {
	if (!isSlotLoaded(activeSlotIndex) || isSlotImage(activeSlotIndex)) {
		return;
	}

	activePlayer().setPaused(false);
	activePlayer().play();
}

void MediaPlaybackEngine::stop() {
	if (isSlotImage(activeSlotIndex)) {
		return;
	}

	PlatformVideo::stopPlayback(activePlayer());
}

bool MediaPlaybackEngine::isLoaded() const {
	return isSlotLoaded(activeSlotIndex);
}

bool MediaPlaybackEngine::isPlaying() const {
	if (isSlotImage(activeSlotIndex)) {
		return false;
	}

	return activePlayer().isPlaying();
}

bool MediaPlaybackEngine::isCurrentClipImage() const {
	return isSlotImage(activeSlotIndex);
}

const MediaClip& MediaPlaybackEngine::currentClip() const {
	static const MediaClip kEmpty;
	if (!clipSource) {
		return kEmpty;
	}
	return clipSource->clipAt(currentClipIndex);
}

void MediaPlaybackEngine::update() {
	++frameCounter;

	if (!isSlotImage(activeSlotIndex) && activePlayer().isLoaded()) {
		activePlayer().update();
	}

	if (prefetchLoadState == PrefetchLoadState::Loading) {
		tickAsyncPrefetch();
	} else if (!isSlotImage(standbySlotIndex()) && standbyPlayer().isLoaded()) {
		const bool deferStandbyPump = activeLoadGraceFrames > 0
			|| (!isSlotImage(activeSlotIndex) && activePlayer().isPlaying()
				&& (frameCounter % kStandbyPumpIntervalFrames) != 0);
		if (!deferStandbyPump) {
			standbyPlayer().update();
		}
	}

	schedulePrefetch();
}

void MediaPlaybackEngine::setImageDrawHints(const ImageDrawHints* hints) {
	imageDrawHints = hints;
}

void MediaPlaybackEngine::draw(const ofRectangle& bounds) const {
	if (isSlotImage(activeSlotIndex)) {
		renderer.draw(imageSlots[activeSlotIndex], bounds, imageDrawHints);
		return;
	}

	renderer.draw(activePlayer(), bounds);
}
