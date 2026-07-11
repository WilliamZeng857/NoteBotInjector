cmake_minimum_required(VERSION 3.16)
project(NoteBotInjector VERSION 3.6.76 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

set(CMAKE_PREFIX_PATH "C:/QtBuild/qt6-static")
option(NB_ENABLE_VMP_MARKERS "Enable VMProtect SDK markers for release artifacts" OFF)
set(NB_VMP_SDK_DIR "" CACHE PATH "Directory that contains VMProtectSDK.h or its parent include directory")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 QuickDialogs2 Network)

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

set(MODEL_DLL_SOURCES
    src__model_dll/src/model_api.cpp
)

set(MODEL_DLL_HEADERS
    src__model_dll/include/notebot_model.h
)

qt_add_executable(NoteBotInjector WIN32
    ${SOURCES}
    ${HEADERS}
    resources__injector_exe/app.qrc
    resources__injector_exe/app_icon.rc
)

add_library(NoteBotModel SHARED
    ${MODEL_DLL_SOURCES}
    ${MODEL_DLL_HEADERS}
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

target_include_directories(NoteBotModel PRIVATE
    src__model_dll/include
)

if(MSVC)
    target_compile_options(NoteBotInjector PRIVATE /utf-8)
    target_compile_options(NoteBotUpdater PRIVATE /utf-8)
    target_compile_options(NoteBotModel PRIVATE /utf-8)
endif()

if(NB_ENABLE_VMP_MARKERS)
    set(_nb_vmp_hint_paths)
    if(NB_VMP_SDK_DIR)
        list(APPEND _nb_vmp_hint_paths "${NB_VMP_SDK_DIR}" "${NB_VMP_SDK_DIR}/include")
    endif()
    if(DEFINED ENV{NB_VMP_SDK_DIR} AND NOT "$ENV{NB_VMP_SDK_DIR}" STREQUAL "")
        list(APPEND _nb_vmp_hint_paths "$ENV{NB_VMP_SDK_DIR}" "$ENV{NB_VMP_SDK_DIR}/include")
    endif()
    find_path(NB_VMP_SDK_INCLUDE_DIR NAMES VMProtectSDK.h HINTS ${_nb_vmp_hint_paths})
    if(NOT NB_VMP_SDK_INCLUDE_DIR)
        message(FATAL_ERROR
            "NB_ENABLE_VMP_MARKERS=ON but VMProtectSDK.h was not found. Set NB_VMP_SDK_DIR.")
    endif()
    foreach(_nb_vmp_target NoteBotInjector NoteBotUpdater NoteBotModel)
        target_compile_definitions(${_nb_vmp_target} PRIVATE NB_VMP_BUILD)
        target_include_directories(${_nb_vmp_target} PRIVATE "${NB_VMP_SDK_INCLUDE_DIR}")
    endforeach()
endif()

set_target_properties(NoteBotInjector PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "NoteBotInjector"
)

set_target_properties(NoteBotUpdater PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "NoteBotUpdater"
)

set_target_properties(NoteBotModel PROPERTIES
    OUTPUT_NAME "NoteBotModel"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/model_runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/model_runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/model_runtime"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/model_runtime"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/model_runtime"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/model_runtime"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/model_runtime"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/model_runtime"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/model_runtime"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/model_runtime"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/model_runtime"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/model_runtime"
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/model_runtime"
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/model_runtime"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/model_runtime"
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

# Generate full linker maps for function-level VMP selection. Keep noinline
# marker leaves alive and unique even when release optimization is enabled.
target_link_options(NoteBotInjector PRIVATE /MAP:NoteBotInjector.map /DEBUG /OPT:NOREF /OPT:NOICF)
target_link_options(NoteBotUpdater PRIVATE /MAP:NoteBotUpdater.map /DEBUG /OPT:NOREF /OPT:NOICF)
target_link_options(NoteBotModel PRIVATE /MAP:NoteBotModel.map /DEBUG /OPT:NOREF /OPT:NOICF)

target_link_libraries(NoteBotInjector PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickDialogs2
    Qt6::Network
)

target_link_libraries(NoteBotUpdater PRIVATE
    Qt6::Core
)

target_link_libraries(NoteBotModel PRIVATE
    Qt6::Core
    Qt6::Gui
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
    target_link_libraries(NoteBotModel PRIVATE
        user32 kernel32 psapi
    )
endif()

# 静态链接需要显式设置 QML 导入路径
set_target_properties(NoteBotInjector PROPERTIES
    QT_QML_IMPORT_PATH ""
)
