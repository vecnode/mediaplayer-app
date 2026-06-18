/*
 * media-player-cpp — MediaCorpusProvider
 * Copyright (c) vecnode 2026
 */

#include "MediaCorpusProvider.h"

void MediaCorpusProvider::setup(const std::string& dataDirectory) {
	loaded_ = corpus_.load_from_directory(dataDirectory);
	if (loaded_) {
		ofLogNotice("MediaCorpusProvider") << "Loaded " << corpus_.size()
			<< " corpus entries from " << dataDirectory;
	} else {
		ofLogWarning("MediaCorpusProvider") << "No corpus markdown in " << dataDirectory
			<< " (expected PDF_TEXT.md and OBJS_TEXT.md)";
	}
}

const metaagent::media::ImageCorpusEntry* MediaCorpusProvider::findForClip(const std::string& clipPath) const {
	if (!loaded_) {
		return nullptr;
	}
	return corpus_.find_by_path(clipPath);
}

std::string MediaCorpusProvider::subtitlePreview(const std::string& clipPath) const {
	if (!loaded_) {
		return {};
	}
	return corpus_.subtitle_preview_for(metaagent::media::corpus_file_key_from_path(clipPath));
}

std::string MediaCorpusProvider::subtitleFull(const std::string& clipPath) const {
	if (!loaded_) {
		return {};
	}
	return corpus_.subtitle_text_for(metaagent::media::corpus_file_key_from_path(clipPath));
}

bool MediaCorpusProvider::focusRectForClip(
	const std::string& clipPath,
	const std::size_t clipIndex,
	metaagent::media::IntRect& outRect) const {
	const metaagent::media::ImageCorpusEntry* entry = findForClip(clipPath);
	if (!entry) {
		return false;
	}

	const std::size_t region_index = corpus_.pick_focus_region_index(*entry, clipIndex);
	return corpus_.region_focus_crop_16x9(*entry, region_index, outRect);
}
