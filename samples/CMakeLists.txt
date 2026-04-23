add_executable(xuanjing-offline-replay
	offline_replay/offline_replay.cpp
)

target_link_libraries(xuanjing-offline-replay
	PRIVATE
		xuanjing-runtime
)

add_executable(xuanjing-benchmark-runner
	benchmark_runner/benchmark_runner.cpp
)

target_link_libraries(xuanjing-benchmark-runner
	PRIVATE
		xuanjing-runtime
		xuanjing-upscale
		xuanjing-genframe
		xuanjing-eval
)
