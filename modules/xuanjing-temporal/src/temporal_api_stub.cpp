#include "xuanjing-temporal/temporal_api.h"

#include <cstdint>
#include <vector>

namespace xuanjing {
namespace temporal {

class DisabledTemporalProcessor : public ITemporalProcessor {
 public:
	bool Prepare(std::uint32_t, std::uint32_t, const TemporalConfig&) override {
		return true;
	}

	bool Process(const runtime::FrameInput&, TemporalResult& result) override {
		result = {};
		return true;
	}

	const char* Name() const override { return "temporal-disabled"; }
};

class SimpleTemporalProcessor : public ITemporalProcessor {
 public:
	bool Prepare(std::uint32_t width, std::uint32_t height,
							 const TemporalConfig& config) override {
		width_ = width;
		height_ = height;
		enableReject_ = config.enableDisocclusionReject;
		historyBuffer_.resize(static_cast<std::size_t>(width_) * height_ * 4U, 0U);
		validityBuffer_.resize(static_cast<std::size_t>(width_) * height_ * 4U, 255U);
		return true;
	}

	bool Process(const runtime::FrameInput& input, TemporalResult& result) override {
		result = {};
		if (!input.hasPrevHistory || input.prevHistory.data == nullptr) {
			return true;
		}

		if (input.prevHistory.width != width_ || input.prevHistory.height != height_) {
			return true;
		}

		for (std::uint32_t y = 0; y < height_; ++y) {
			const auto* src =
					static_cast<const std::uint8_t*>(input.prevHistory.data) +
					y * input.prevHistory.rowStrideBytes;
			auto* dst = historyBuffer_.data() + static_cast<std::size_t>(y) * width_ * 4U;
			for (std::uint32_t x = 0; x < width_; ++x) {
				const bool reject = enableReject_ && input.motionVectors.data != nullptr &&
														input.motionVectors.width > x &&
														input.motionVectors.height > y &&
														static_cast<const std::uint8_t*>(input.motionVectors.data)
																		[y * input.motionVectors.rowStrideBytes + x * 4] >
																220U;
				if (reject) {
					dst[x * 4 + 0] = 0U;
					dst[x * 4 + 1] = 0U;
					dst[x * 4 + 2] = 0U;
					dst[x * 4 + 3] = 255U;
					validityBuffer_[(static_cast<std::size_t>(y) * width_ + x) * 4U + 0] = 0U;
					validityBuffer_[(static_cast<std::size_t>(y) * width_ + x) * 4U + 1] = 0U;
					validityBuffer_[(static_cast<std::size_t>(y) * width_ + x) * 4U + 2] = 0U;
					validityBuffer_[(static_cast<std::size_t>(y) * width_ + x) * 4U + 3] = 255U;
				} else {
					dst[x * 4 + 0] = src[x * 4 + 0];
					dst[x * 4 + 1] = src[x * 4 + 1];
					dst[x * 4 + 2] = src[x * 4 + 2];
					dst[x * 4 + 3] = src[x * 4 + 3];
					validityBuffer_[(static_cast<std::size_t>(y) * width_ + x) * 4U + 0] = 255U;
					validityBuffer_[(static_cast<std::size_t>(y) * width_ + x) * 4U + 1] = 255U;
					validityBuffer_[(static_cast<std::size_t>(y) * width_ + x) * 4U + 2] = 255U;
					validityBuffer_[(static_cast<std::size_t>(y) * width_ + x) * 4U + 3] = 255U;
				}
			}
		}

		historyView_.data = historyBuffer_.data();
		historyView_.width = width_;
		historyView_.height = height_;
		historyView_.rowStrideBytes = static_cast<std::size_t>(width_) * 4U;
		historyView_.format = runtime::PixelFormat::kRGBA8UNorm;
		historyView_.colorSpace = input.prevHistory.colorSpace;

		validityView_.data = validityBuffer_.data();
		validityView_.width = width_;
		validityView_.height = height_;
		validityView_.rowStrideBytes = static_cast<std::size_t>(width_) * 4U;
		validityView_.format = runtime::PixelFormat::kRGBA8UNorm;
		validityView_.colorSpace = runtime::ColorSpace::kLinear;

		result.reprojectedHistory = historyView_;
		result.validityMask = validityView_;
		result.hasReprojectedHistory = true;
		return true;
	}

	const char* Name() const override { return "temporal-simple"; }

 private:
	std::uint32_t width_ = 0;
	std::uint32_t height_ = 0;
	bool enableReject_ = true;
	std::vector<std::uint8_t> historyBuffer_;
	std::vector<std::uint8_t> validityBuffer_;
	runtime::ImageView historyView_{};
	runtime::ImageView validityView_{};
};

ITemporalProcessor* CreateDisabledTemporalProcessor() {
	return new DisabledTemporalProcessor();
}

ITemporalProcessor* CreateSimpleTemporalProcessor() {
	return new SimpleTemporalProcessor();
}

}  // namespace temporal
}  // namespace xuanjing
