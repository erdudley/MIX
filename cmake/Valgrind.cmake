find_program(VALGRIND_EXECUTABLE valgrind)

function(mix_add_valgrind_target target_name test_executable)
    if(NOT VALGRIND_EXECUTABLE)
        message(STATUS "valgrind not found; target '${target_name}' will be a stub")
        add_custom_target(${target_name}
            COMMAND ${CMAKE_COMMAND} -E echo
                "valgrind is not installed; install it to run memory checks"
        )
        return()
    endif()

    set(log_dir "${CMAKE_BINARY_DIR}/valgrind")
    set(suppressions "${CMAKE_SOURCE_DIR}/cmake/valgrind.supp")

    set(valgrind_cmd
        ${VALGRIND_EXECUTABLE}
        --leak-check=full
        --show-leak-kinds=all
        --track-origins=yes
        --error-exitcode=1
        --log-file=${log_dir}/%p.log
    )
    if(EXISTS "${suppressions}")
        file(READ "${suppressions}" _mix_valgrind_supp)
        if(_mix_valgrind_supp MATCHES "\\{")
            list(APPEND valgrind_cmd --suppressions=${suppressions})
        endif()
    endif()
    list(APPEND valgrind_cmd $<TARGET_FILE:${test_executable}>)

    add_custom_target(${target_name}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${log_dir}"
        COMMAND ${valgrind_cmd}
        DEPENDS ${test_executable}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Run unit tests under Valgrind (logs in ${log_dir}/)"
        VERBATIM
    )
endfunction()