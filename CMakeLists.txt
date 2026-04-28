cmake_minimum_required (VERSION 3.13)

project(SDI12Analyzer)

add_definitions( -DLOGIC2 )

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum supported MacOS version" FORCE)

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/SDI12Analyzer.cpp
src/SDI12Analyzer.h
src/SDI12AnalyzerResults.cpp
src/SDI12AnalyzerResults.h
src/SDI12AnalyzerSettings.cpp
src/SDI12AnalyzerSettings.h
src/SDI12SimulationDataGenerator.cpp
src/SDI12SimulationDataGenerator.h
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})
