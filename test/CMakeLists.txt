add_executable(${PROJECT_NAME}_test
    "test/assets/rectangle_64x32.c"
    "test/cases/compositing_over_premultiplied.c"
    "test/tools/rgba2png.c"
    "test/tools/str.c"
    "test/main.c"
)

target_link_libraries(${PROJECT_NAME}_test
    PRIVATE
        ${PROJECT_NAME}
        cutest
)

target_include_directories(${PROJECT_NAME}_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test
)

setup_target_wall(${PROJECT_NAME}_test)
