

# Automatically include all .cpp and .h files from the src/ directory
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

option(USE_SHADER_LANGUAGE "Choose shader language: SLANG or GLSL" "GLSL")

# Create the executable
add_executable(SandBox ${SRC_FILES})

set_target_properties(SandBox PROPERTIES FOLDER "Misc")

# Define VK_NO_PROTOTYPES to avoid including Vulkan prototypes
# This is necessary because we are using volk to load Vulkan functions
target_compile_definitions(SandBox PRIVATE YAML_CPP_STATIC_DEFINE VK_NO_PROTOTYPES USE_SLANG IMGUI_IMPL_VULKAN_USE_VOLK)
target_compile_definitions(VanK PRIVATE YAML_CPP_STATIC_DEFINE VK_NO_PROTOTYPES USE_SLANG IMGUI_IMPL_VULKAN_USE_VOLK)

 if(USE_SHADER_LANGUAGE STREQUAL "SLANG")
       target_compile_definitions(VanK PUBLIC USE_SLANG)
       target_compile_definitions(SandBox PRIVATE USE_SLANG)
   endif()

# Link the static library (libmath)
target_link_libraries(SandBox PRIVATE VanK)

target_include_directories(SandBox PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Define the path to SDL3.dll
set(SDL3_DLL_PATH "${CMAKE_SOURCE_DIR}/VanK/Vendor/SDL3/lib/x64/SDL3.dll")

# Copy SDL3.dll to the executable output directory
add_custom_command(TARGET SandBox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_DLL_PATH}"
        $<TARGET_FILE_DIR:SandBox>  # Copy it to the directory where the .exe is built
)

add_custom_command(TARGET SandBox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/VanK/Vendor/Slang/bin/slang.dll"
        $<TARGET_FILE_DIR:SandBox>
)

add_custom_command(TARGET SandBox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/VanK/Vendor/Slang/bin/slang-glslang.dll"
        $<TARGET_FILE_DIR:SandBox>
)

# Define the path to SDL3-Image.dll
set(SDL3_Image_DLL_PATH "${CMAKE_SOURCE_DIR}/VanK/Vendor/SDL3-Image/lib/x64/SDL3_image.dll")

# Copy SDL3-Image.dll to the executable output directory
add_custom_command(TARGET SandBox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_Image_DLL_PATH}"
        $<TARGET_FILE_DIR:SandBox>  # Copy it to the directory where the .exe is built
)

get_target_property(_incs SandBox INCLUDE_DIRECTORIES)
message(STATUS "SandBox include dirs: ${_incs}")

get_target_property(_libs SandBox LINK_LIBRARIES)
message(STATUS "SandBox link libs: ${_libs}")