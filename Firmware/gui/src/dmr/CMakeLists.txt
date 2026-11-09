# The supplied library headers retain their upstream <dmr/...> namespace.
# Stage them in the build tree without changing the vendored sources.
foreach(header burst data embedded phy symbol vocoder voice)
    configure_file(src/${header}.h include/dmr/${header}.h COPYONLY)
endforeach()

add_library(brass_dmr_core STATIC
    src/burst.c src/data.c src/embedded.c src/voice.c
    src/phy/discriminator.c src/phy/matched_filter.c src/phy/rx.c
    src/phy/symbol.c src/phy/sync.c src/phy/sync_detector.c src/phy/tx.c
)
target_include_directories(brass_dmr_core PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(brass_dmr_core PUBLIC liquid m)

add_library(brass_dmr_vocoder STATIC
    vocoder/vocoder.c vocoder/portable_analysis.c vocoder/portable_encoder.c
    vocoder/portable_encoder_analysis.c vocoder/portable_encoder_callback.c
    vocoder/portable_fixed.c vocoder/portable_frame.c
    vocoder/portable_initial_state.c vocoder/portable_pitch.c
)
target_include_directories(brass_dmr_vocoder PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
target_compile_features(brass_dmr_core PRIVATE c_std_11)
target_compile_features(brass_dmr_vocoder PRIVATE c_std_11)

# Fixed-point vocoder state overlays and arithmetic require the same semantics
# at -O3 as in the reference build (do not apply these flags to the GUI/PHY).
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(brass_dmr_vocoder PRIVATE -fno-strict-aliasing -fwrapv)
endif()
