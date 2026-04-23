#pragma once

namespace xuanjing::model {

enum class ModelFormat {
	kONNX,
};

struct ModelPackage {
	const char* modelId = nullptr;
	const char* modelPath = nullptr;
	ModelFormat format = ModelFormat::kONNX;
};

struct ModelStatus {
	bool success = false;
	const char* message = nullptr;
};

bool RegisterModelPackage(const ModelPackage& package,
													ModelStatus* status = nullptr);

bool ResolveModelPath(const char* modelId,
											const char** modelPath,
											ModelFormat* format = nullptr,
											ModelStatus* status = nullptr);

bool RegisterDataset(const char* dataset_name);

}  // namespace xuanjing::model
