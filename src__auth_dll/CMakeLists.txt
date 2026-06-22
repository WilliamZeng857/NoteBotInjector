cmake_minimum_required(VERSION 3.16)
project(NoteBotAuth VERSION 3.5.51 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

set(CMAKE_PREFIX_PATH "C:/QtBuild/qt6-static")
option(NB_AUTH_ENABLE_VMP_MARKERS "Enable VMProtect SDK markers for Auth DLL" OFF)
set(NB_VMP_SDK_DIR "" CACHE PATH "Directory that contains VMProtectSDK.h or its parent include directory")

find_package(Qt6 REQUIRED COMPONENTS Core Network)

set(SOURCES
    src/api.cpp
    ../src__injector_exe/win32injector.cpp
    src/crypto/crypto_utils.cpp
    src/protected/protected_download_ops.cpp
    src/protected/protected_name_ops.cpp
    src/protected/protected_ticket_ops.cpp
    src/protected/protected_verify_ops.cpp
    src/protected/protected_v3_ops.cpp
    src/name_injector.cpp
    src/v3/v3_state.cpp
    src/v3/v3_actions.cpp
    src/v3/v3_rpc_client.cpp
)

set(HEADERS
    include/notebot_auth.h
    src/crypto/crypto_utils.h
    src/crypto/secure_string.h
    src/crypto/vmp_defs.h
    src/protected/protected_download_ops.h
    src/protected/protected_name_ops.h
    src/protected/protected_ticket_ops.h
    src/protected/protected_verify_ops.h
    src/protected/protected_v3_ops.h
    src/name_injector.h
    src/v3/v3_state.h
    src/v3/v3_actions.h
    src/v3/v3_rpc_client.h
)

add_library(NoteBotAuth SHARED
    ${SOURCES}
    ${HEADERS}
    version_info.rc
)

target_include_directories(NoteBotAuth PRIVATE
    include
    ../src__injector_exe
    src
    src/crypto
    src/protected
    src/v3
)
target_compile_definitions(NoteBotAuth PRIVATE NB_AUTH_EXPORTS)

if(NB_AUTH_ENABLE_VMP_MARKERS)
    set(_nb_vmp_hint_paths)
    if(NB_VMP_SDK_DIR)
        list(APPEND _nb_vmp_hint_paths
            "${NB_VMP_SDK_DIR}"
            "${NB_VMP_SDK_DIR}/include"
        )
    endif()
    if(DEFINED ENV{NB_VMP_SDK_DIR} AND NOT "$ENV{NB_VMP_SDK_DIR}" STREQUAL "")
        list(APPEND _nb_vmp_hint_paths
            "$ENV{NB_VMP_SDK_DIR}"
            "$ENV{NB_VMP_SDK_DIR}/include"
        )
    endif()

    find_path(NB_VMP_SDK_INCLUDE_DIR
        NAMES VMProtectSDK.h
        HINTS ${_nb_vmp_hint_paths}
    )
    if(NOT NB_VMP_SDK_INCLUDE_DIR)
        message(FATAL_ERROR
            "NB_AUTH_ENABLE_VMP_MARKERS=ON but VMProtectSDK.h was not found. "
            "Set NB_VMP_SDK_DIR to the SDK root or include directory.")
    endif()

    target_compile_definitions(NoteBotAuth PRIVATE NB_VMP_BUILD)
    target_include_directories(NoteBotAuth PRIVATE "${NB_VMP_SDK_INCLUDE_DIR}")
endif()

if(MSVC)
    target_compile_options(NoteBotAuth PRIVATE /utf-8 /Zo)
endif()

target_link_libraries(NoteBotAuth PRIVATE
    Qt6::Core
    Qt6::Network
)

if(WIN32)
    target_link_libraries(NoteBotAuth PRIVATE
        user32 kernel32 gdi32 shell32 advapi32 ole32
        ws2_32 iphlpapi crypt32 netapi32 winmm
        version winspool
        bcrypt ncrypt urlmon winhttp
    )
endif()

# 生成 MAP 文件用于 VMP
set_target_properties(NoteBotAuth PROPERTIES
    LINK_FLAGS "/MAP:NoteBotAuth.map /MAPINFO:EXPORTS /DEBUG /OPT:NOREF /OPT:NOICF"
)
