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
    857859285d1bc25a1dc1977ceb6268a918865a0e379bf100dc9e4722005563f2
    d6f774fc57b17f77f732c27cd7c4ca0e64f781ac68694569abf9e97369232c43)
isle_prepare_post_course_source(report prepare_report.py
    d0917bfdf2910be85ce5f179744a102228577a9b33a314ac59a811720047f357
    9245c8066d69f5b6ee30dc651065cd0f508b261a026a6ab706c2daa91c7700bb)
isle_prepare_post_course_source(gallery prepare_gallery.py
    415b955dd3be508f9b84c2159ae6f4c83337db1ee3ed0de3480669fe01b9465f
    aaeab29e6853afe89cbf1c6207521db1fd9572cc4140f003cb6cbad5f1853cfc)
isle_prepare_post_course_source(end-level-8 prepare_end_level_8.py
    cfe2689bcfceae343c4bf4a1f3ae2f64b853e2db4d47f1e68b4629bd82299b8c
    58f9b7c151665816d104d093c903a6edff110bcd3963e65476e5c0b1dec0aa6d)
isle_prepare_post_course_source(credits prepare_credits.py
    48fcee61dedca6bbba13982a1bb4bc0164210d95178644532fc6dc600d68cf35
    4bff0ff764a054beb28fc0a5c52b1c7b1ba8325611b91634410199d503ee13b5)
isle_prepare_post_course_source(original-sort prepare_original_sort.py
    42364399b6b78b023ce393d3cd3b141b930229dcd8ecbfee3ddc037771ca4294
    abcaf6af309c144a0a8f13d0fae5a8f67e377950c9e5699304d492c6f9adc30e)
isle_prepare_post_course_source(photo-check materialize_photo_check.py
    de71afa30ace2ec56c3ae3057fc0beef635cfa0e019b9bef087fb3aaf1ef1c3d
    c92961a9e498d768732d23d1a0f0018b8534a35c972338c804fa3b2516472858)
isle_prepare_post_course_source(printer-diagnostic prepare_printer_diagnostic.py
    65bbadd50b6a3be6f7010e54d7e162c362ddef5bcf7605646a0ba13aaa5d6294
    4808e16f07b3596f195b09d65d7d57f42ef9117f6a89e52fd720f97b556c9535)
