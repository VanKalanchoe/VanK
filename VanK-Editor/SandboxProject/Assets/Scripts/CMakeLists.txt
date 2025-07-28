cmake_minimum_required(VERSION 3.28)

project(Sandbox VERSION 1.0 LANGUAGES CXX CSharp)

# Create the managed C# library
file(GLOB_RECURSE SCRIPTCORE_CS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cs"
    "${CMAKE_CURRENT_SOURCE_DIR}/Properties/*.cs"
)

add_library(Sandbox SHARED ${SCRIPTCORE_CS_FILES})

target_link_libraries(Sandbox PRIVATE VanK-ScriptCore)

# Set .NET Framework version
set_property(TARGET Sandbox PROPERTY
    VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2"
)

# Add .NET references
set_property(TARGET Sandbox PROPERTY
    VS_DOTNET_REFERENCES "System;System.Core;"
)

# Copy the DLL next to output
if (SCRIPTCORE_PATH)
    add_custom_command(TARGET Sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPTCORE_PATH}"
            "$<TARGET_FILE_DIR:Sandbox>/VanK-ScriptCore.dll"
    )
endif()

set_target_properties(Sandbox PROPERTIES
    # Put all runtime outputs directly inside this folder (no Debug/Release subfolders)
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/VanK-Editor/Resources/Scripts"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/VanK-Editor/Resources/Scripts"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/VanK-Editor/Resources/Scripts"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/VanK-Editor/Resources/Scripts"

    # Put intermediate files here, no config subfolders
    VS_GLOBAL_BaseIntermediateOutputPath "${CMAKE_BINARY_DIR}/VanK-Editor/Resources/Scripts/Intermediates"
)