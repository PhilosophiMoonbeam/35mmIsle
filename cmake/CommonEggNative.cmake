# Include once after isle-course-native-host exists. CommonEggSource.cmake
# adds the six Original C definitions and C bridge to isle-engine separately.
if(NOT TARGET isle-course-native-host)
    message(FATAL_ERROR "Common native Egg requires the actual course native Host")
endif()
target_sources(isle-course-native-host PRIVATE
    "${PROJECT_SOURCE_DIR}/src/course_actor_egg_native.cpp"
    "${PROJECT_SOURCE_DIR}/src/course_actor_egg_native.hpp"
    "${PROJECT_SOURCE_DIR}/src/egg_source.h")
