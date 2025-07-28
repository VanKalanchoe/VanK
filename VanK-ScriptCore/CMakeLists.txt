
# Create the managed C# library
file(GLOB_RECURSE SCRIPTCORE_CS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cs"
    "${CMAKE_CURRENT_SOURCE_DIR}/Properties/*.cs"
)

add_library(VanK-ScriptCore SHARED ${SCRIPTCORE_CS_FILES})

# Set .NET Framework version
set_property(TARGET VanK-ScriptCore PROPERTY
    VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2"
)

# Add .NET references
set_property(TARGET VanK-ScriptCore PROPERTY
    VS_DOTNET_REFERENCES "System;System.Core"
)

set_target_properties(VanK-ScriptCore PROPERTIES
    FOLDER "Core"

    # Put all runtime outputs directly inside this folder (no Debug/Release subfolders)
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/VanK-Editor/Resources/Scripts"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/VanK-Editor/Resources/Scripts"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/VanK-Editor/Resources/Scripts"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/VanK-Editor/Resources/Scripts"

    # Put intermediate files here, no config subfolders
    VS_GLOBAL_BaseIntermediateOutputPath "${CMAKE_BINARY_DIR}/VanK-Editor/Resources/Scripts/Intermediates"
)