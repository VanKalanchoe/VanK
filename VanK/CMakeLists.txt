# Automatically include all .cpp and .h files from the src/ directory
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

# Create the static library
add_library(VanK STATIC ${SRC_FILES})

set_target_properties(VanK PROPERTIES FOLDER "Core")

# Export the include directory for other targets
target_include_directories(VanK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(VanK PUBLIC "${CMAKE_SOURCE_DIR}/VanK-Editor/assets/shaders")

target_include_directories(VanK PUBLIC "${CMAKE_SOURCE_DIR}/VanK")

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CTEST_TESTS_DISABLED ON CACHE BOOL "" FORCE)

#custom idk if this is good ------------------------------
# Download CPM.cmake
set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/fetchContent/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake...")
    file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()
include(${CPM_DOWNLOAD_LOCATION})

set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/fetchContent/CPMCache")

# Include Volk
CPMAddPackage(
  NAME volk
  GITHUB_REPOSITORY zeux/volk
  GIT_TAG 1.4.304
)
set_property(TARGET volk PROPERTY FOLDER "Dependencies")

# Include VMA
CPMAddPackage(
  NAME VulkanMemoryAllocator
  GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
  GIT_TAG master
  DOWNLOAD_ONLY
)
set(VMA_SOURCE ${VulkanMemoryAllocator_SOURCE_DIR}/include/vk_mem_alloc.h)
target_include_directories(VanK PRIVATE ${VulkanMemoryAllocator_SOURCE_DIR}/include)
target_sources(VanK PRIVATE ${VMA_SOURCE})
source_group("vma" FILES ${VMA_SOURCE})

CPMAddPackage(
  NAME box2d
  GITHUB_REPOSITORY erincatto/box2d
  VERSION 3.1.1
)
set_property(TARGET box2d PROPERTY FOLDER "Dependencies")

CPMAddPackage(
  NAME glm
  GITHUB_REPOSITORY g-truc/glm
  GIT_TAG 1.0.1
)
set_property(TARGET glm PROPERTY FOLDER "Dependencies")

CPMAddPackage(
  NAME imgui
  GITHUB_REPOSITORY ocornut/imgui
  GIT_TAG v1.91.8-docking
)
#set_property(TARGET imgui PROPERTY FOLDER "Dependencies")

# Add ImGui source files to the project
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    #${IMGUimgui_SOURCE_DIRI_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${VOLK_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/SDL3/include )

target_compile_definitions(imgui PRIVATE IMGUI_DEFINE_MATH_OPERATORS IMGUI_IMPL_VULKAN_USE_VOLK)
target_link_libraries(imgui PUBLIC volk)
set_property(TARGET imgui PROPERTY FOLDER "Dependencies")

CPMAddPackage(
  NAME ImGuizmo
  GITHUB_REPOSITORY CedricGuillemet/ImGuizmo
  GIT_TAG master
  DOWNLOAD_ONLY YES
)

add_library(ImGuizmo STATIC 
    ${ImGuizmo_SOURCE_DIR}/ImGuizmo.cpp
)

target_include_directories(ImGuizmo PUBLIC ${ImGuizmo_SOURCE_DIR} ${imgui_SOURCE_DIR})

CPMAddPackage(
  NAME spdlog
  GITHUB_REPOSITORY gabime/spdlog
  GIT_TAG v1.15.1
)

CPMAddPackage(
  NAME yaml-cpp
  GITHUB_REPOSITORY jbeder/yaml-cpp
  GIT_TAG 0.8.0
  OPTIONS
    "YAML_CPP_BUILD_TESTS OFF"
    "YAML_CPP_BUILD_TOOLS OFF"
    "YAML_CPP_INSTALL OFF"
)
set_property(TARGET yaml-cpp PROPERTY FOLDER "Dependencies")

# Link libraries and include directories
target_link_libraries(VanK PUBLIC 
  volk
  box2d
  imgui
  ImGuizmo
  glm
  spdlog
  yaml-cpp
)

#maybe sdl3 in the future convert to cpmpackage

#maybe do this idk for yamlcpp
# Link different versions of YAML-CPP based on the build configuration
#target_link_libraries(VanK PRIVATE 
#    $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Vendor/yaml-cpp/build-debug/Debug/yaml-cppd.lib>
#    $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/Vendor/yaml-cpp/build-release/Release/yaml-cpp.lib>
#    $<$<CONFIG:RelWithDebInfo>:${CMAKE_CURRENT_SOURCE_DIR}/Vendor/yaml-cpp/build-release/Release/RelWithDebInfo/yaml-cpp.lib>
#    $<$<CONFIG:MinSizeRel>:${CMAKE_CURRENT_SOURCE_DIR}/Vendor/yaml-cpp/build-release/Release/yaml-cpp.lib>
#)

#custom-----------------------------------------------

# Download stb_image.h
set(STB_IMAGE_URL "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h")
set(STB_IMAGE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/stb_image/stb_image.h")
if(NOT EXISTS ${STB_IMAGE_PATH})
    message(STATUS "Downloading stb_image.h...")
    file(DOWNLOAD ${STB_IMAGE_URL} ${STB_IMAGE_PATH})
endif()
target_sources(VanK PRIVATE ${STB_IMAGE_PATH})
source_group("stb" FILES ${STB_IMAGE_PATH})
target_include_directories(VanK PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

#### 3 Party libs SDL3 ####
target_include_directories(VanK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/SDL3/include)

# Optionally specify the version of SDL3 you are using
set(SDL3_VERSION 3.2.4)

# Link the SDL3 static library
target_link_libraries(VanK PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/SDL3/lib/x64/SDL3.lib")

#### 3 Party libs Entt ####
target_include_directories(VanK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Entt/include)

# Optionally specify the version of Entt you are using
set(Entt_VERSION 3.14.0)

#### 3 Party libs mono ####
target_include_directories(VanK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/mono/include)

# Optionally specify the version of mono you are using
set(mono_VERSION 6.12.0.182)

# Link the mono static library
target_link_libraries(VanK PRIVATE 
    "$<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Vendor/mono/lib/Debug/libmono-static-sgen.lib>"
    "$<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/Vendor/mono/lib/Release/libmono-static-sgen.lib>")

#### 3 Party libs FileWatch ####
target_include_directories(VanK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/FileWatch/include)

# Optionally specify the version of FileWatch you are using
set(FileWatch_VERSION 1.0.0)

#### 3 Party libs xxhash ####
target_include_directories(VanK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/xxhash/include)

# Füge die xxhash-Quelldatei hinzu
target_sources(VanK PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/xxhash/xxhash.c
)

# Optionally specify the version of FileWatch you are using
set(xxhash_VERSION 0.8.3)

#### 3 Party libs SDL3 Image ####
target_include_directories(VanK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/SDL3-Image/include)

# Optionally specify the version of SDL3-Image you are using
set(SDL3-Image_VERSION 3.1.0)
# Link the SDL3-Image static library
target_link_libraries(VanK PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/SDL3-Image/lib/x64/SDL3_image.lib")

# Full path to the assets directory
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/VanK-Editor/assets")

# IMAGE FILES
file(GLOB IMAGE_FILES "${ASSETS_DIR}/textures/*.jpg")
source_group("Resources" FILES ${IMAGE_FILES})
add_custom_target(Resources SOURCES ${IMAGE_FILES})

# SHADER FILES
file(GLOB SHADER_H_FILES "${ASSETS_DIR}/shaders/*.h")
file(GLOB SHADER_SLANG_FILES "${ASSETS_DIR}/shaders/*.slang")

# Combine shaders and group
set(SHADER_FILES ${SHADER_H_FILES} ${SHADER_SLANG_FILES})
source_group("Shaders" FILES ${SHADER_FILES})
add_custom_target(Shaders SOURCES ${SHADER_FILES})

#### 3 Party libs Slang ####
target_include_directories(VanK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Slang/include)

# Link the Slang static library
target_link_libraries(VanK PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Slang/lib/slang.lib")

# Optionally specify the version of Slang you are using
set(Slang_VERSION 2025.12.1)

get_target_property(_incs VanK INCLUDE_DIRECTORIES)
message(STATUS "VanK include dirs: ${_incs}")

get_target_property(_libs VanK LINK_LIBRARIES)
message(STATUS "VanK link libs: ${_libs}")

# Windows only dont have to do this if you link mono dynamicly

# Link the Ws2_32 library for mono or error "WinSock"
target_link_libraries(VanK PRIVATE "Ws2_32.lib")

# Link the Winmm library for mono or error "Windows Multi Media"
target_link_libraries(VanK PRIVATE "Winmm.lib")

# Link the Version library for mono or error "Win Version"
target_link_libraries(VanK PRIVATE "Version.lib")

# Link the Bcrypt library for mono or error "Bcrypt"
target_link_libraries(VanK PRIVATE "Bcrypt.lib")