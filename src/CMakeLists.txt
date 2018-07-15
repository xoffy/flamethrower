
set(SOURCES main.c flamethrower.c picture.c util.c noise.c)
add_executable(flamethrower ${SOURCES})
target_link_libraries(flamethrower m)
