#pragma once

#include "media/corpus.hpp"
#include "ofMain.h"
#include <string>

/// Loads PDF_TEXT.md + OBJS_TEXT.md from the media-player data folder (lazy).
class MediaCorpusProvider {
public:
	void setDataDirectory(const std::string& dataDirectory);

	bool isLoaded() const;
	std::size_t entryCount() const;

	const metaagent::media::ImageCorpusEntry* findForClip(const std::string& clipPath) const;
	std::string subtitlePreview(const std::string& clipPath) const;
	std::string subtitleSummary(const std::string& clipPath) const;
	std::string subtitleFull(const std::string& clipPath) const;

	/// Index of a random detected region for the clip (valid bbox only).
	std::size_t pickRandomRegionIndex(const std::string& clipPath) const;

	bool focusRectForRegion(
		const std::string& clipPath,
		std::size_t regionIndex,
		float viewportAspectWidthOverHeight,
		metaagent::media::IntRect& outRect) const;

	bool regionBboxForClip(
		const std::string& clipPath,
		std::size_t regionIndex,
		metaagent::media::IntRect& outBbox) const;

	std::size_t regionCountForClip(const std::string& clipPath) const;
	std::size_t entriesWithRegionsCount() const;

private:
	void ensureLoaded() const;

	mutable metaagent::media::MediaCorpus corpus_;
	mutable bool loaded_ = false;
	mutable bool loadAttempted_ = false;
	std::string dataDirectory_;
};
