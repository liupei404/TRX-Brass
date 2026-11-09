add_library(sstv STATIC
    src/color.c
    src/decoder.c
    src/encoder.c
    src/goertzel.c
    src/modes.c
)

target_include_directories(sstv PUBLIC include)
target_link_libraries(sstv PUBLIC liquid m)
target_link_libraries(${PROJECT_NAME} PRIVATE sstv)
