cmake_minimum_required(VERSION 3.28)

project(VanK VERSION 1.0 LANGUAGES CXX CSharp)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options("/utf-8")

add_compile_definitions($<$<CONFIG:Debug>:VK_DEBUG>)

# Add subdirectories
add_subdirectory(VanK)
add_subdirectory(SandBox)
add_subdirectory(VanK-Editor)
add_subdirectory(VanK-ScriptCore)
add_subdirectory(VanK-Editor/SandboxProject/Assets/Scripts)
add_subdirectory(Facerun/Assets/Scripts)

# Get OS name (lowercase)
string(TOLOWER "${CMAKE_SYSTEM_NAME}" os_name)

# Compose folder name based on generator platform and system name
string(TOLOWER "${CMAKE_SYSTEM_NAME}" os_name)

# Use CMAKE_GENERATOR_PLATFORM if available; fallback to processor
if(CMAKE_GENERATOR_PLATFORM)
    string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" arch)
else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" arch)
endif()

# Targets to configure
set(MY_TARGETS VanK SandBox VanK-Editor VanK-ScriptCore Sandbox Facerun)

# Loop over multi-config types
foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOLOWER "${CONFIG_TYPE}" config_name)
    
    set(OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin/${config_name}-${os_name}-${arch}")
    set(INT_PATH "${CMAKE_BINARY_DIR}/bin-int/${config_name}-${os_name}-${arch}")

    foreach(target ${MY_TARGETS})
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${OUTPUT_PATH}"
            LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${OUTPUT_PATH}"
            ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${INT_PATH}"
            PDB_OUTPUT_DIRECTORY_${CONFIG_TYPE}     "${INT_PATH}"
        )
    endforeach()
endforeach()