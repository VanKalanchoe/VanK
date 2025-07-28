# Automatically include all .cpp and .h files from the src/ directory
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

option(USE_SHADER_LANGUAGE "Choose shader language: SLANG or GLSL" "GLSL")
   
# Create the executable
add_executable(VanK-Editor ${SRC_FILES})

# make all folder visisble in ide
# 1. Glob all files under current dir (including src/)
file(GLOB_RECURSE ALL_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*"
)

set(ADDITIONAL_FILES "")
foreach(file_path IN LISTS ALL_FILES)
    if(NOT IS_DIRECTORY "${file_path}")
        # Only files NOT under src/
        string(FIND "${file_path}" "${CMAKE_CURRENT_SOURCE_DIR}/src/" is_src)
        if(is_src EQUAL -1)
            list(APPEND ADDITIONAL_FILES "${file_path}")
        endif()
    endif()
endforeach()

# 2. Organize additional files in source groups by folder (for VS)
foreach(file_path IN LISTS ADDITIONAL_FILES)
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}" "${file_path}")
    get_filename_component(dir "${rel_path}" DIRECTORY)
    string(REPLACE "/" "\\" group "${dir}")
    source_group("${group}" FILES "${file_path}")
endforeach()

# 3. Add these files as sources to the VanK-Editor target
target_sources(VanK-Editor PRIVATE ${ADDITIONAL_FILES})
#-------------------------------------------------------

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

set(SOURCE_CONTENT_DIR "${CMAKE_SOURCE_DIR}/VanK-Editor/assets/content")
set(DESTINATION_CONTENT_DIR "${CMAKE_BINARY_DIR}/VanK-Editor/content")

if(NOT EXISTS ${CMAKE_BINARY_DIR}/content_copied.stamp)
    file(COPY ${SOURCE_CONTENT_DIR} DESTINATION ${DESTINATION_CONTENT_DIR})
    file(TOUCH ${CMAKE_BINARY_DIR}/content_copied.stamp)
endif()

get_target_property(_incs VanK-Editor INCLUDE_DIRECTORIES)
message(STATUS "VanK-Editor include dirs: ${_incs}")

get_target_property(_libs VanK-Editor LINK_LIBRARIES)
message(STATUS "VanK-Editor link libs: ${_libs}")