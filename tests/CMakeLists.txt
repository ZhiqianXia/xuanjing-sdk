
# Integration tests
if(XUANJING_BUILD_SAMPLES)
	add_executable(tests_pipeline_integration_test
		integration/pipeline_integration_test.cpp
		../modules/xuanjing-runtime/src/pipeline_orchestrator.cpp
	)

	target_link_libraries(tests_pipeline_integration_test
		PRIVATE
			xuanjing-runtime
			xuanjing-temporal
			xuanjing-upscale
			xuanjing-shader
			xuanjing-tensor
			xuanjing-eval
			xuanjing-model
	)

	add_test(
		NAME pipeline_integration
		COMMAND tests_pipeline_integration_test
	)

	# Performance benchmarks
	add_executable(tests_shader_perf_benchmark
		perf/shader_perf_benchmark.cpp
	)

	target_link_libraries(tests_shader_perf_benchmark
		PRIVATE
			xuanjing-runtime
			xuanjing-shader
			xuanjing-upscale
			xuanjing-eval
	)

	add_test(
		NAME shader_perf_benchmark
		COMMAND tests_shader_perf_benchmark
	)
endif()
