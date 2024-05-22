cmake_minimum_required(VERSION 3.5)
project(SurveillanceSystem)
find_package(PkgConfig REQUIRED)
# Options for building parts of the model processing library
option(USE_TVM "Compile with TVM support" OFF)
option(USE_TENSOR_LITE "Compile with TensorLite support" OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Set compiler optimization flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -flto -ffunction-sections -fdata-sections -g -std=c11 -fPIC -Wall -Wextra -march=armv8-a -mtune=cortex-a53")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -flto -ffunction-sections -fdata-sections -g -std=c++17 -fPIC -Wall -Wextra -march=armv8-a -mtune=cortex-a53")
# Set linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# Include directories for header files
include_directories(${PROJECT_SOURCE_DIR}/include)
# Library for logging
add_library(logger
    Logger.cpp
)
#  Link the necessary libraries for logging
target_link_libraries(logger
    log4cplus
)
# Library for frame processing
add_library(framehandler
    CameraFrameHandler.cpp
)

# Link the necessary libraries for frame processing
target_link_libraries(framehandler
    opencv_core
    opencv_imgproc
    opencv_imgcodecs
    streamerconsumer
)

# Library for model processing
if(ENABLE_CLASSIFICATION)
add_library(modelprocessor
    ObjectClassifier.cpp
)
if(USE_TVM)
    target_sources(modelprocessor PRIVATE TVMRunner.cpp)
    target_compile_definitions(modelprocessor PRIVATE USE_TVM)
    target_link_libraries(modelprocessor
        tvm_runtime
    )
endif()

if(USE_TENSOR_LITE)
    set(_TFLITE_LIB_ ${CMAKE_INSTALL_PREFIX}/lib/_tensorflow_lite_)
    set(RTMESSAGE_PATH ${CMAKE_INSTALL_PREFIX}/src/rbus/src/rtmessage)
    target_sources(modelprocessor PRIVATE TensorLiteRunner.cpp)
    target_compile_definitions(modelprocessor PRIVATE USE_TENSOR_LITE)
    link_directories(${_TFLITE_LIB_})
    target_link_libraries(modelprocessor
    logger
    tensorflow-lite
    absl_bad_any_cast_impl
    absl_bad_optional_access
    absl_bad_variant_access
    absl_base
    absl_city
    absl_civil_time
    absl_cord
    absl_cord_internal
    absl_cordz_functions
    absl_cordz_handle
    absl_cordz_info
    absl_cordz_sample_token
    absl_crc32c
    absl_crc_cord_state
    absl_crc_cpu_detect
    absl_crc_internal
    absl_debugging_internal
    absl_demangle_internal
    absl_die_if_null
    absl_examine_stack
    absl_exponential_biased
    absl_failure_signal_handler
    absl_flags
    absl_flags_commandlineflag
    absl_flags_commandlineflag_internal
    absl_flags_config
    absl_flags_internal
    absl_flags_marshalling
    absl_flags_parse
    absl_flags_private_handle_accessor
    absl_flags_program_name
    absl_flags_reflection
    absl_flags_usage
    absl_flags_usage_internal
    absl_graphcycles_internal
    absl_hash
    absl_hashtablez_sampler
    absl_int128
    absl_leak_check
    absl_log_entry
    absl_log_flags
    absl_log_globals
    absl_log_initialize
    absl_log_internal_check_op
    absl_log_internal_conditions
    absl_log_internal_format
    absl_log_internal_globals
    absl_log_internal_log_sink_set
    absl_log_internal_message
    absl_log_internal_nullguard
    absl_log_internal_proto
    absl_log_severity
    absl_log_sink
    absl_low_level_hash
    absl_malloc_internal
    absl_periodic_sampler
    absl_random_distributions
    absl_random_internal_distribution_test_util
    absl_random_internal_platform
    absl_random_internal_pool_urbg
    absl_random_internal_randen
    absl_random_internal_randen_hwaes
    absl_random_internal_randen_hwaes_impl
    absl_random_internal_randen_slow
    absl_random_internal_seed_material
    absl_random_seed_gen_exception
    absl_random_seed_sequences
    absl_raw_hash_set
    absl_raw_logging_internal
    absl_scoped_set_env
    absl_spinlock_wait
    absl_stacktrace
    absl_status
    absl_statusor
    absl_strerror
    absl_str_format_internal
    absl_strings
    absl_strings_internal
    absl_symbolize
    absl_synchronization
    absl_throw_delegate
    absl_time
    absl_time_zone
    #fft2d_alloc
    #fft2d_fft4f2d
    fft2d_fftsg2d
    #fft2d_fftsg3d
    fft2d_fftsg
    #fft2d_shrtdct
    flatbuffers
    ruy_ctx
    ruy_allocator
    ruy_frontend
    ruy_context_get_ctx
    ruy_context
    ruy_apply_multiplier
    ruy_prepacked_cache
    ruy_tune
    ruy_cpuinfo
    ruy_system_aligned_alloc
    ruy_prepare_packed_matrices
    ruy_trmul
    ruy_block_map
    ruy_denormal
    ruy_thread_pool
    ruy_blocking_counter
    ruy_wait
    ruy_kernel_arm
    ruy_pack_arm
    #ruy_have_built_path_for_avx2_fma
    #ruy_have_built_path_for_avx512
    #ruy_have_built_path_for_avx
    #ruy_kernel_avx2_fma
    #ruy_kernel_avx512
    #ruy_kernel_avx
    #ruy_pack_avx2_fma
    #ruy_pack_avx512
    #ruy_pack_avx
    ##ruy_profiler_instrumentation
    ##ruy_profiler_profiler
    farmhash
    XNNPACK
    pthreadpool
    cpuinfo
    cpuinfo_internals
    eight_bit_int_gemm
    )
endif()

# Create executable
add_executable(surveillanceApp MotionEventMetadata.cpp PredictionProcessor.cpp ThumbnailGenerater.cpp SurveillanceSystem.cpp RTMessageBroker.cpp main.cpp)
# Link libraries to the executable
target_link_libraries(surveillanceApp
    framehandler
    modelprocessor
    rtMessage
)
else()
# Create executable
add_executable(surveillanceApp MotionEventMetadata.cpp ThumbnailGenerater.cpp SurveillanceSystem.cpp RTMessageBroker.cpp main.cpp)
# Link libraries to the executable
target_link_libraries(surveillanceApp
    framehandler
    rtMessage
    logger
)
endif()

