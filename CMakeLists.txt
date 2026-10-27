cmake_minimum_required(VERSION 3.20)

project(systemfeur VERSION 1.0 LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed, use: cmake -B build")
endif()

option(SYSTEMFEUR_STATIC "Link systemfeur as a fully static binary" ON)
option(SYSTEMFEUR_WERROR "Treat compiler warnings as errors" ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(
        CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel
    )
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(
    SYSTEMFEUR_FETCH_STATIC_LIBS
    "Download static libc/libstdc++ (no root needed) when the system lacks them"
    ON
)
set(SYSTEMFEUR_STATIC_LIBDIR
    "${PROJECT_SOURCE_DIR}/.static-libs"
    CACHE PATH
    "Extra directory searched for static libraries"
)

if(SYSTEMFEUR_STATIC)
    include(CheckCXXSourceCompiles)
    include(cmake/FetchStaticLibs.cmake)

    function(systemfeur_check_static_libs result)
        set(link_options -static)
        if(IS_DIRECTORY "${SYSTEMFEUR_STATIC_LIBDIR}")
            list(APPEND link_options "-L${SYSTEMFEUR_STATIC_LIBDIR}")
        endif()
        set(CMAKE_REQUIRED_LINK_OPTIONS ${link_options})
        unset(${result} CACHE)
        check_cxx_source_compiles(
            "#include <iostream>\nint main() { std::cout << 1; }"
            ${result}
        )
        set(SYSTEMFEUR_STATIC_LINK_OPTIONS ${link_options} PARENT_SCOPE)
    endfunction()

    systemfeur_check_static_libs(SYSTEMFEUR_HAVE_STATIC_LIBS)

    if(NOT SYSTEMFEUR_HAVE_STATIC_LIBS AND SYSTEMFEUR_FETCH_STATIC_LIBS)
        fetch_static_libs("${SYSTEMFEUR_STATIC_LIBDIR}" fetched)
        if(fetched)
            systemfeur_check_static_libs(SYSTEMFEUR_HAVE_STATIC_LIBS)
        endif()
    endif()

    if(NOT SYSTEMFEUR_HAVE_STATIC_LIBS)
        message(
            FATAL_ERROR
            "Static libc/libstdc++ not found. Install them (Fedora: glibc-static libstdc++-static) "
            "or configure with -DSYSTEMFEUR_STATIC=OFF"
        )
    endif()
endif()

add_executable(systemfeur src/main.cpp)

target_compile_options(
    systemfeur
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        $<$<BOOL:${SYSTEMFEUR_WERROR}>:-Werror>
)

if(SYSTEMFEUR_STATIC)
    target_link_options(systemfeur PRIVATE ${SYSTEMFEUR_STATIC_LINK_OPTIONS})
endif()

include(GNUInstallDirs)
install(TARGETS systemfeur RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
