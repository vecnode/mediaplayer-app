#pragma once

#include "media/corpus.hpp"
#include "ofMain.h"
#include <string>

/// Loads PDF_TEXT.md + OBJS_TEXT.md from the media-player data folder.
class MediaCorpusProvider {
public:
	void setup(const std::string& dataDirectory);

	bool isLoaded() const { return loaded_; }
	std::size_t entryCount() const { return corpus_.size(); }

	const metaagent::media::ImageCorpusEntry* findForClip(const std::string& clipPath) const;
	std::string subtitlePreview(const std::string& clipPath) const;
	std::string subtitleFull(const std::string& clipPath) const;
	bool focusRectForClip(
		const std::string& clipPath,
		std::size_t clipIndex,
		metaagent::media::IntRect& outRect) const;

private:
	metaagent::media::MediaCorpus corpus_;
	bool loaded_ = false;
};
