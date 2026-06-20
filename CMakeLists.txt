cmake_minimum_required(VERSION 3.16)
project(NoteBotInjector VERSION 3.6.76 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

set(CMAKE_PREFIX_PATH "C:/QtBuild/qt6-static")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network)

set(SOURCES
    src__injector_exe/main.cpp
    src__injector_exe/modelcatalogmodel.cpp
    src__injector_exe/modelpreviewitem.cpp
    src__injector_exe/win32process.cpp
    src__injector_exe/win32injector.cpp
    src__injector_exe/processmodel.cpp
    src__injector_exe/logmodel.cpp
    src__injector_exe/backend.cpp
    src__injector_exe/updater.cpp
    src__auth_dll/src/crypto/crypto_utils.cpp
)

set(HEADERS
    src__injector_exe/modelcatalogmodel.h
    src__injector_exe/modelpreviewitem.h
    src__injector_exe/win32process.h
    src__injector_exe/win32injector.h
    src__injector_exe/processmodel.h
    src__injector_exe/logmodel.h
    src__injector_exe/backend.h
)

qt_add_executable(NoteBotInjector WIN32
    ${SOURCES}
    ${HEADERS}
    resources__injector_exe/app.qrc
    resources__injector_exe/app_icon.rc
)

add_executable(NoteBotUpdater WIN32
    src__updater_exe/updater_main.cpp
    src__updater_exe/version_info.rc
)

target_include_directories(NoteBotInjector PRIVATE
    src__injector_exe
    src__auth_dll/include
    src__auth_dll/src/crypto
    C:/QtBuild/qt6-static/include/QtGui/6.11.0/QtGui
)

target_include_directories(NoteBotUpdater PRIVATE
    src__updater_exe
)

if(MSVC)
    target_compile_options(NoteBotInjector PRIVATE /utf-8)
    target_compile_options(NoteBotUpdater PRIVATE /utf-8)
endif()

set_target_properties(NoteBotInjector PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "NoteBotInjector"
)

set_target_properties(NoteBotUpdater PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "NoteBotUpdater"
)

set(NB_EMBEDDED_UPDATER_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/resources__injector_exe/embedded/NoteBotUpdater.exe")

add_custom_command(
    OUTPUT "${NB_EMBEDDED_UPDATER_PATH}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/resources__injector_exe/embedded"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:NoteBotUpdater>
            "${NB_EMBEDDED_UPDATER_PATH}"
    DEPENDS NoteBotUpdater
    VERBATIM
)

add_custom_target(NoteBotUpdaterEmbeddedResource ALL
    DEPENDS "${NB_EMBEDDED_UPDATER_PATH}"
)

add_dependencies(NoteBotInjector NoteBotUpdaterEmbeddedResource)

# 生成 MAP 文件用于 VMP 加壳分析
target_link_options(NoteBotInjector PRIVATE /MAP:NoteBotInjector.map)

target_link_libraries(NoteBotInjector PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
)

target_link_libraries(NoteBotUpdater PRIVATE
    Qt6::Core
)

if(WIN32)
    target_link_libraries(NoteBotInjector PRIVATE
        user32 kernel32 gdi32 shell32 advapi32 ole32
        ws2_32 iphlpapi crypt32 netapi32 winmm
        opengl32 imm32 version winspool
        bcrypt ncrypt urlmon
    )
    target_link_libraries(NoteBotUpdater PRIVATE
        user32 kernel32 shell32 advapi32 ole32
    )
endif()

# 静态链接需要显式设置 QML 导入路径
set_target_properties(NoteBotInjector PROPERTIES
    QT_QML_IMPORT_PATH ""
)
