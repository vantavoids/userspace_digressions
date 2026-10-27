function(fetch_static_libs dest result_var)
    set(${result_var} FALSE PARENT_SCOPE)

    find_program(FETCH_STATIC_LIBS_RPM rpm)
    find_program(FETCH_STATIC_LIBS_DNF dnf)
    if(NOT FETCH_STATIC_LIBS_RPM OR NOT FETCH_STATIC_LIBS_DNF)
        message(STATUS "Cannot fetch static libraries: rpm or dnf not found")
        return()
    endif()

    set(arch "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    set(packages "")

    foreach(lib IN ITEMS glibc libstdc++)
        execute_process(
            COMMAND "${FETCH_STATIC_LIBS_RPM}" -q --qf "%{VERSION}-%{RELEASE}" "${lib}.${arch}"
            OUTPUT_VARIABLE version
            RESULT_VARIABLE rc
        )
        if(NOT rc EQUAL 0)
            message(STATUS "Cannot fetch static libraries: ${lib}.${arch} is not installed")
            return()
        endif()
        list(APPEND packages "${lib}-static-${version}.${arch}")
    endforeach()

    set(work "${CMAKE_BINARY_DIR}/fetch-static-libs")
    file(REMOVE_RECURSE "${work}")

    list(JOIN packages " " packages_str)
    message(STATUS "Downloading ${packages_str}")
    execute_process(
        COMMAND "${FETCH_STATIC_LIBS_DNF}" download -q "--destdir=${work}" ${packages}
        RESULT_VARIABLE rc
    )
    if(NOT rc EQUAL 0)
        message(STATUS "Cannot fetch static libraries: dnf download failed")
        file(REMOVE_RECURSE "${work}")
        return()
    endif()

    file(GLOB rpms "${work}/*.rpm")
    foreach(rpm IN LISTS rpms)
        file(ARCHIVE_EXTRACT INPUT "${rpm}" DESTINATION "${work}/root" PATTERNS "*.a")
    endforeach()

    file(REMOVE_RECURSE "${dest}")
    file(GLOB_RECURSE archives "${work}/root/*.a")
    file(COPY ${archives} DESTINATION "${dest}")
    file(REMOVE_RECURSE "${work}")

    # libm.a is a linker script with absolute /usr/lib64 paths; make them bare
    # names so the linker resolves them through the -L search path instead.
    file(READ "${dest}/libm.a" libm)
    string(REGEX REPLACE "/[^ \n]*/(lib[^ /\n]*\\.a)" "\\1" libm "${libm}")
    file(WRITE "${dest}/libm.a" "${libm}")

    message(STATUS "Static libraries installed in ${dest}")
    set(${result_var} TRUE PARENT_SCOPE)
endfunction()
