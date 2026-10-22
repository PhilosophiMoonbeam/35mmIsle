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
    if(name STREQUAL "end-level-8")
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
    elseif(name STREQUAL "end-level-8")
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
    0db24d25282bb3ce58b8049b61d766434956ed1e21b8ef5109fd22a89cc06194
    07dcc3fc1c9a49cc04aa464c0dffb865ee67677615c4f58cf138318681370180)
isle_prepare_post_course_source(report prepare_report.py
    d0917bfdf2910be85ce5f179744a102228577a9b33a314ac59a811720047f357
    383b145d04f7e6882cf3b3f0d3def78c9cb4dd150a0b4ed5850a4a109b9ebb5d)
isle_prepare_post_course_source(gallery prepare_gallery.py
    415b955dd3be508f9b84c2159ae6f4c83337db1ee3ed0de3480669fe01b9465f
    16272b210ed4e7b383b97fbff3d9dfe386fc338b58c823d3e4099139005dd0cf)
isle_prepare_post_course_source(end-level-8 prepare_end_level_8.py
    cfe2689bcfceae343c4bf4a1f3ae2f64b853e2db4d47f1e68b4629bd82299b8c
    bc683119a2f19f117657a8f5b7a1eb33b1315480ca0bc9f16dcff297c849f633)
isle_prepare_post_course_source(credits prepare_credits.py
    48fcee61dedca6bbba13982a1bb4bc0164210d95178644532fc6dc600d68cf35
    9ba267951c7e5d9a0ed2577340535205b23dfbfd65615d3542ae2724eb533ba9)
isle_prepare_post_course_source(original-sort prepare_original_sort.py
    42364399b6b78b023ce393d3cd3b141b930229dcd8ecbfee3ddc037771ca4294
    abcaf6af309c144a0a8f13d0fae5a8f67e377950c9e5699304d492c6f9adc30e)
isle_prepare_post_course_source(photo-check materialize_photo_check.py
    de71afa30ace2ec56c3ae3057fc0beef635cfa0e019b9bef087fb3aaf1ef1c3d
    eee5386f4d0ee4bd292a4cde0a6c76c4fe920aa73809ec764d5e836f4a58c9cf)
