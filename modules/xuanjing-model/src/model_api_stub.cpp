#include "xuanjing-model/model_api.h"

#include <filesystem>
#include <string>
#include <vector>

namespace xuanjing::model {

namespace {

struct StoredPackage {
	std::string modelId;
	std::string modelPath;
	ModelFormat format = ModelFormat::kONNX;
};

std::vector<StoredPackage>& Registry() {
	static std::vector<StoredPackage> packages;
	return packages;
}

void FillStatus(ModelStatus* status, bool ok, const char* message) {
	if (status == nullptr) {
		return;
	}
	status->success = ok;
	status->message = message;
}

bool IsNullOrEmpty(const char* s) {
	return s == nullptr || s[0] == '\0';
}

}  // namespace

bool RegisterModelPackage(const ModelPackage& package, ModelStatus* status) {
	if (IsNullOrEmpty(package.modelId)) {
		FillStatus(status, false, "modelId is null or empty");
		return false;
	}
	if (IsNullOrEmpty(package.modelPath)) {
		FillStatus(status, false, "modelPath is null or empty");
		return false;
	}

	const std::filesystem::path path(package.modelPath);
	if (!std::filesystem::exists(path)) {
		FillStatus(status, false, "modelPath does not exist");
		return false;
	}

	auto& packages = Registry();
	for (auto& item : packages) {
		if (item.modelId == package.modelId) {
			if (item.modelPath == package.modelPath && item.format == package.format) {
				FillStatus(status, true, "already registered");
				return true;
			}

			item.modelPath = package.modelPath;
			item.format = package.format;
			FillStatus(status, true, "updated existing registration");
			return true;
		}
	}

	packages.push_back(StoredPackage{
			package.modelId,
			package.modelPath,
			package.format,
	});
	FillStatus(status, true, "registered");
	return true;
}

bool ResolveModelPath(const char* modelId,
											const char** modelPath,
											ModelFormat* format,
											ModelStatus* status) {
	if (modelPath == nullptr) {
		FillStatus(status, false, "modelPath output pointer is null");
		return false;
	}
	if (IsNullOrEmpty(modelId)) {
		FillStatus(status, false, "modelId is null or empty");
		return false;
	}

	const auto& packages = Registry();
	for (const auto& item : packages) {
		if (item.modelId == modelId) {
			*modelPath = item.modelPath.c_str();
			if (format != nullptr) {
				*format = item.format;
			}
			FillStatus(status, true, "resolved");
			return true;
		}
	}

	FillStatus(status, false, "modelId is not registered");
	return false;
}

bool RegisterDataset(const char* dataset_name) { return dataset_name != nullptr; }

}  // namespace xuanjing::model
