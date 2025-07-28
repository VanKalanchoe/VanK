# Automatically include all .cpp and .h files from the src/ directory
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

option(USE_SHADER_LANGUAGE "Choose shader language: SLANG or GLSL" "GLSL")
   
# Create the executable
add_executable(VanK-Editor ${SRC_FILES})

set_target_properties(VanK-Editor PROPERTIES FOLDER "Tools" VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:VanK-Editor>/../")

# Define VK_NO_PROTOTYPES to avoid including Vulkan prototypes
# This is necessary because we are using volk to load Vulkan functions
target_compile_definitions(VanK-Editor PRIVATE YAML_CPP_STATIC_DEFINE VK_NO_PROTOTYPES USE_SLANG)
target_compile_definitions(VanK PRIVATE YAML_CPP_STATIC_DEFINE VK_NO_PROTOTYPES USE_SLANG)

 if(USE_SHADER_LANGUAGE STREQUAL "SLANG")
       target_compile_definitions(VanK PUBLIC USE_SLANG)
       target_compile_definitions(VanK-Editor PRIVATE USE_SLANG)
   endif()
   

# Link the static library (libmath)
target_link_libraries(VanK-Editor PRIVATE VanK)

target_include_directories(VanK-Editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Define the path to SDL3.dll
set(SDL3_DLL_PATH "${CMAKE_SOURCE_DIR}/VanK/Vendor/SDL3/lib/x64/SDL3.dll")

# Copy SDL3.dll to the executable output directory
add_custom_command(TARGET VanK-Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_DLL_PATH}"
        $<TARGET_FILE_DIR:VanK-Editor>  # Copy it to the directory where the .exe is built
)

add_custom_command(TARGET VanK-Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/VanK/Vendor/Slang/bin/slang.dll"
        $<TARGET_FILE_DIR:VanK-Editor>
)

add_custom_command(TARGET VanK-Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/VanK/Vendor/Slang/bin/slang-glslang.dll"
        $<TARGET_FILE_DIR:VanK-Editor>
)

# Define the path to SDL3-Image.dll
set(SDL3_Image_DLL_PATH "${CMAKE_SOURCE_DIR}/VanK/Vendor/SDL3-Image/lib/x64/SDL3_image.dll")

# Copy SDL3-Image.dll to the executable output directory
add_custom_command(TARGET VanK-Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_Image_DLL_PATH}"
        $<TARGET_FILE_DIR:VanK-Editor>  # Copy it to the directory where the .exe is built
)

add_custom_command(TARGET VanK-Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/VanK-Editor/assets/imgui.ini"
        $<TARGET_FILE_DIR:VanK-Editor>/../
)

add_custom_command(TARGET VanK-Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/VanK-Editor/assets/content"
        "$<TARGET_FILE_DIR:VanK-Editor>/content"
)

get_target_property(_incs VanK-Editor INCLUDE_DIRECTORIES)
message(STATUS "VanK-Editor include dirs: ${_incs}")

get_target_property(_libs VanK-Editor LINK_LIBRARIES)
message(STATUS "VanK-Editor link libs: ${_libs}")