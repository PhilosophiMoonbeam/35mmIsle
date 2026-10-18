# Preserve file-level notices from the actual selected build inputs. This is
# not R1/R2 release approval: embedded terms, linkage and corresponding-source
# obligations still require review of the complete candidate composition.
function(isle_install_runtime_notices root label)
    if(NOT IS_DIRECTORY "${root}")
        return()
    endif()
    file(GLOB_RECURSE notices LIST_DIRECTORIES FALSE "${root}/*")
    foreach(notice IN LISTS notices)
        file(RELATIVE_PATH relative "${root}" "${notice}")
        get_filename_component(name "${notice}" NAME)
        string(TOLOWER "${relative}" lower_relative)
        string(TOLOWER "${name}" lower_name)
        if(lower_name MATCHES "\\.(c|cc|cpp|cxx|h|hh|hpp|hxx|s)$")
            continue()
        endif()
        if(lower_name MATCHES "^(license|copying|copyright|notice|credits)"
           OR lower_name MATCHES "\\.license$"
           OR lower_relative MATCHES "(^|/)(licenses|license)/")
            get_filename_component(directory "${relative}" DIRECTORY)
            install(FILES "${notice}" DESTINATION
                "${CMAKE_INSTALL_DATADIR}/licenses/35mmisle/dependencies/${label}/${directory}"
                COMPONENT Runtime)
        endif()
    endforeach()
endfunction()

isle_install_runtime_notices("${_isle_lus_private}" libultraship)
isle_install_runtime_notices("${ISLE_RSP_NOTICES}" parallel-rsp)
isle_install_runtime_notices("${_isle_rdp_private}/notices" parallel-rdp)
foreach(dependency IN ITEMS ImGui ThreadPool prism monocypher)
    FetchContent_GetProperties(${dependency} SOURCE_DIR notice_source)
    isle_install_runtime_notices("${notice_source}" "${dependency}")
endforeach()
foreach(prefix IN LISTS CMAKE_PREFIX_PATH)
    isle_install_runtime_notices("${prefix}/share/doc" sdk)
    isle_install_runtime_notices("${prefix}/share/licenses" sdk)
endforeach()
if(VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
    isle_install_runtime_notices("${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share" vcpkg)
endif()
