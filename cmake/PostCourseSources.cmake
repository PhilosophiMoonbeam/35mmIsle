# Private Original C outputs are admitted before becoming compilation inputs.
# These external generator/manifest pins are independent of recipe support hashes.
function(isle_prepare_post_course_source name generator generator_sha256 manifest_sha256)
    string(REPLACE "-" "_" module "${name}")
    set(manifest "${PROJECT_SOURCE_DIR}/cmake/${name}-inputs.json")
    set(generator "${PROJECT_SOURCE_DIR}/tools/${generator}")
    set(private "${CMAKE_BINARY_DIR}/_deps/isle-${name}-private")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${generator}" "${manifest}")
    file(SHA256 "${generator}" actual_generator_sha256)
    file(SHA256 "${manifest}" actual_manifest_sha256)
    if(NOT actual_generator_sha256 STREQUAL generator_sha256)
        message(FATAL_ERROR "Unadmitted ${name} generator fingerprint")
    endif()
    if(NOT actual_manifest_sha256 STREQUAL manifest_sha256)
        message(FATAL_ERROR "Unadmitted ${name} manifest fingerprint")
    endif()
    file(READ "${manifest}" authority)
    if(name STREQUAL "end-level-8" OR name STREQUAL "printer-diagnostic")
        set(support_key support_sha256)
    else()
        set(support_key native_support)
    endif()
    string(JSON support_count LENGTH "${authority}" "${support_key}")
    if(support_count GREATER 0)
        math(EXPR support_last "${support_count} - 1")
        foreach(support_index RANGE 0 ${support_last})
            string(JSON support_path MEMBER "${authority}" "${support_key}" ${support_index})
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                "${PROJECT_SOURCE_DIR}/${support_path}")
        endforeach()
    endif()
    if(name STREQUAL "album")
        # Album admits its canonical recipe seal internally, not a CLI digest.
        set(arguments --source-repo "${_isle_snap_source}" --support-root "${PROJECT_SOURCE_DIR}"
            --build-root "${CMAKE_BINARY_DIR}" --manifest "${manifest}")
    elseif(name STREQUAL "end-level-8" OR name STREQUAL "printer-diagnostic")
        set(arguments --decomp "${_isle_snap_source}" --support-root "${PROJECT_SOURCE_DIR}"
            --recipe "${manifest}" --manifest-sha256 "${manifest_sha256}" --output "${private}")
    elseif(name STREQUAL "credits")
        set(arguments --source-repo "${_isle_snap_source}" --native-root "${PROJECT_SOURCE_DIR}"
            --manifest "${manifest}" --manifest-sha256 "${manifest_sha256}" --output "${private}")
    elseif(name STREQUAL "photo-check")
        # The materializer publishes guarded game.c beside this include output.
        set(arguments --source-repo "${_isle_snap_source}" --manifest "${manifest}"
            --manifest-sha256 "${manifest_sha256}" --output "${private}/photo_check_original.inc")
    else()
        set(arguments --source-repo "${_isle_snap_source}" --manifest "${manifest}"
            --manifest-sha256 "${manifest_sha256}" --output "${private}/game.c")
    endif()
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -B "${generator}" ${arguments}
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Exact ${name} C admission failed:\n${output}${error}")
    endif()
    set(_isle_${module}_private "${private}" PARENT_SCOPE)
    set(_isle_${module}_generated "${private}/game.c" PARENT_SCOPE)
endfunction()

isle_prepare_post_course_source(album prepare_album.py
    1707b7e94af8fc66f2609766e0adb74cfb7e01e7b68eb1217576607ad68650c6
    8455501eed479cfcf7a080a8d765a1308d36228974757634f9a902eb795aaa46)
isle_prepare_post_course_source(report prepare_report.py
    d0917bfdf2910be85ce5f179744a102228577a9b33a314ac59a811720047f357
    5233b6eef1e4f4c589ecf3b03596f98bca3fd94739779e39eac6d1d78ccdd141)
isle_prepare_post_course_source(gallery prepare_gallery.py
    baa9efed74dd76cb041238f23a1f2e4ba9df06a44f88259dccae74f7d543188a
    64a86eb72559b9bc18b72fb584b791ad73697906504c2859f987dcf46efb5e07)
isle_prepare_post_course_source(end-level-8 prepare_end_level_8.py
    0cce63beb08078d06bfc40194dd55fea0fec88aebb4ff82f57ca976f8d7b6e68
    9f96856465473233de32855221aebeab6c53f163dcd56d383e408ebbced2aae0)
isle_prepare_post_course_source(credits prepare_credits.py
    48fcee61dedca6bbba13982a1bb4bc0164210d95178644532fc6dc600d68cf35
    51efcd1954c2096c6945cd5310db08a5b84b34232ed5a6ad14ce3fbec042d5f3)
isle_prepare_post_course_source(original-sort prepare_original_sort.py
    42364399b6b78b023ce393d3cd3b141b930229dcd8ecbfee3ddc037771ca4294
    abcaf6af309c144a0a8f13d0fae5a8f67e377950c9e5699304d492c6f9adc30e)
isle_prepare_post_course_source(photo-check materialize_photo_check.py
    de71afa30ace2ec56c3ae3057fc0beef635cfa0e019b9bef087fb3aaf1ef1c3d
    14c6b73c753085ef4f46008550d5e3f6d12e4af2f377c4737da4fe3ee79846b6)
isle_prepare_post_course_source(printer-diagnostic prepare_printer_diagnostic.py
    65bbadd50b6a3be6f7010e54d7e162c362ddef5bcf7605646a0ba13aaa5d6294
    c30bf0ce7f9842c1f89869f14adea3b3cd61b6f5de476187666ad4fa8c8305bd)
