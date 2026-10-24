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
    f3e0c67845d2d8ad61f9524cb4e7f2dd341f3047d09fc805b206985b35209de8
    f4c1fd3766074d37e1dc7ce76a06dc32284aead471372321259e881ab08cace6)
isle_prepare_post_course_source(report prepare_report.py
    d0917bfdf2910be85ce5f179744a102228577a9b33a314ac59a811720047f357
    f502592e83668ccf356f9844bd2476edac4e47ace30f687084ede133e871488e)
isle_prepare_post_course_source(gallery prepare_gallery.py
    baa9efed74dd76cb041238f23a1f2e4ba9df06a44f88259dccae74f7d543188a
    106672e3f36ef0e49b040f3c73b74bf4f1c92b74d7155240894a4d0fd24e363f)
isle_prepare_post_course_source(end-level-8 prepare_end_level_8.py
    0cce63beb08078d06bfc40194dd55fea0fec88aebb4ff82f57ca976f8d7b6e68
    347f19dd2810fadb25097948abb2b9eaa0cd4bafac062dcf6543f539f641ec4d)
isle_prepare_post_course_source(credits prepare_credits.py
    48fcee61dedca6bbba13982a1bb4bc0164210d95178644532fc6dc600d68cf35
    d01736a14a66dd285cb0dcd53755249c97a3636d8b1178e4e98f9c07866c74c4)
isle_prepare_post_course_source(original-sort prepare_original_sort.py
    42364399b6b78b023ce393d3cd3b141b930229dcd8ecbfee3ddc037771ca4294
    abcaf6af309c144a0a8f13d0fae5a8f67e377950c9e5699304d492c6f9adc30e)
isle_prepare_post_course_source(photo-check materialize_photo_check.py
    de71afa30ace2ec56c3ae3057fc0beef635cfa0e019b9bef087fb3aaf1ef1c3d
    30aa22db40c31e65d94d581e455507cb7e45a49464173935863f375885d2d3fe)
isle_prepare_post_course_source(printer-diagnostic prepare_printer_diagnostic.py
    65bbadd50b6a3be6f7010e54d7e162c362ddef5bcf7605646a0ba13aaa5d6294
    182d8afefa3f898335a4322ca9b0e6bd356b7fc3639122708ca58399a095e5b7)
