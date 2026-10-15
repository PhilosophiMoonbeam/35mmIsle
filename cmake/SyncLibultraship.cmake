# Synchronize already-validated LUS inputs, never use live bytes to admit a source.
# The inventory owns deletion only: upstream-generated live files are not inputs.
function(isle_sync_lus_source staging live inventory previous_inputs)
    file(GLOB_RECURSE _inputs LIST_DIRECTORIES true RELATIVE "${staging}" "${staging}/*")
    list(SORT _inputs)
    # git ls-tree -r at 759b13401cd6903daa67a00a753066f791622e78 contains
    # only mode-100644 blobs (no symlinks or submodules). Fail closed if an
    # adaptation changes that shape, rather than dereference a link while copying.
    foreach(_relative IN LISTS _inputs)
        if(IS_SYMLINK "${staging}/${_relative}")
            message(FATAL_ERROR "The pinned LUS input must not contain symlinks: ${_relative}")
        endif()
    endforeach()

    if(IS_SYMLINK "${live}" OR (EXISTS "${live}" AND NOT IS_DIRECTORY "${live}"))
        file(REMOVE_RECURSE "${live}")
    endif()
    file(MAKE_DIRECTORY "${live}")

    # Walk actual live paths without following symlink directories. Delete only
    # obsolete owned inputs, children first; a removed input directory containing
    # generated artifacts must survive. The previous archive bootstraps ownership
    # for builds created before this inventory existed.
    file(GLOB_RECURSE _live_paths LIST_DIRECTORIES true RELATIVE "${live}" "${live}/*")
    list(SORT _live_paths ORDER DESCENDING)
    foreach(_relative IN LISTS _live_paths)
        if(_relative IN_LIST previous_inputs AND NOT _relative IN_LIST _inputs)
            set(_destination "${live}/${_relative}")
            if(IS_SYMLINK "${_destination}" OR NOT IS_DIRECTORY "${_destination}")
                file(REMOVE "${_destination}")
            else()
                file(GLOB _children "${_destination}/*")
                if(NOT _children)
                    file(REMOVE_RECURSE "${_destination}")
                endif()
            endif()
        endif()
    endforeach()

    # Parents sort before children. Replace wrong live types before descending so
    # copying never writes through a live symlink. Only changed regular-file bytes
    # receive new timestamps; COPY_FILE does not register transient staging inputs
    # as configure dependencies. This API is available before our CMake 3.24 floor.
    foreach(_relative IN LISTS _inputs)
        set(_source "${staging}/${_relative}")
        set(_destination "${live}/${_relative}")
        if(IS_DIRECTORY "${_source}")
            if(IS_SYMLINK "${_destination}" OR
                    (EXISTS "${_destination}" AND NOT IS_DIRECTORY "${_destination}"))
                file(REMOVE_RECURSE "${_destination}")
            endif()
            file(MAKE_DIRECTORY "${_destination}")
        else()
            if(IS_SYMLINK "${_destination}" OR IS_DIRECTORY "${_destination}")
                file(REMOVE_RECURSE "${_destination}")
            endif()
            file(COPY_FILE "${_source}" "${_destination}" ONLY_IF_DIFFERENT)
        endif()
    endforeach()

    string(REPLACE ";" "\n" _inventory_text "${_inputs}")
    file(CONFIGURE OUTPUT "${inventory}" CONTENT "${_inventory_text}\n" @ONLY)
endfunction()
