#pragma once

#include "xuanjing-runtime/runtime_api.h"

namespace xuanjing {
namespace temporal {

struct TemporalConfig {
	bool enableDisocclusionReject = true;
};

struct TemporalResult {
	runtime::ImageView reprojectedHistory;
	runtime::ImageView validityMask;
	bool hasReprojectedHistory = false;
};

class ITemporalProcessor {
 public:
	virtual ~ITemporalProcessor() = default;

	virtual bool Prepare(std::uint32_t width, std::uint32_t height,
											 const TemporalConfig& config) = 0;
	virtual bool Process(const runtime::FrameInput& input,
											 TemporalResult& result) = 0;
	virtual const char* Name() const = 0;
};

ITemporalProcessor* CreateDisabledTemporalProcessor();
ITemporalProcessor* CreateSimpleTemporalProcessor();

}  // namespace temporal
}  // namespace xuanjing
