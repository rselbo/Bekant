cmake_minimum_required(VERSION 3.16)

project(Bekant VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 COMPONENTS Quick REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS QuickControls2)

set(APP_ICON_RESOURCE_WINDOWS bekant.rc)
qt_add_resources(RESOURCES bekant.qrc)
qt_add_executable(appBekant
    main.cpp
    bekant.h
    bekant.cpp
    bekantnetwork.h
    bekantnetwork.cpp
    memorypositionlist.h
    memorypositionlist.cpp
    memorypositionmodel.h
    memorypositionmodel.cpp
    ${RESOURCES}
    ${APP_ICON_RESOURCE_WINDOWS}
)

set_source_files_properties(qml/Values.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(appBekant
    URI Bekant
    VERSION 1.0
    QML_FILES main.qml qml/Values.qml qml/BekantButton.qml qml/BekantInput.qml qml/BekantMemoryPosition.qml
)

set_target_properties(appBekant PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_compile_definitions(appBekant
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(appBekant
    PRIVATE
    Qt6::Quick
    Qt6::Network
    Qt6::QuickControls2
)
