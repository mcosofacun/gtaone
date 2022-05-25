add_subdirectory(src)
add_executable(gtaone ${GTAONE_SRC})

set_target_properties(gtaone PROPERTIES CXX_STANDARD 17)
set_target_properties(gtaone PROPERTIES CXX_STANDARD_REQUIRED ON)

target_precompile_headers(gtaone PUBLIC src/stdafx.h)

if(APPLE)
    add_definitions(-D"GL_SILENCE_DEPRECATION")
endif()
find_package(OpenAL REQUIRED)

if(WITH_BOX2D)
    set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "Skip tests")
    set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "Skip testbed")

    add_subdirectory(third_party/Box2D)

    target_include_directories(gtaone PUBLIC "third_party/Box2D/include/")
    target_link_libraries(gtaone "${CMAKE_BINARY_DIR}/third_party/Box2D/bin/libbox2d.a")

    add_dependencies(gtaone box2d)
else()
    find_package(Box2D 2.4.0 REQUIRED)
    target_include_directories(gtaone PUBLIC ${BOX2D_INCLUDE_DIR})
    target_link_libraries(gtaone ${BOX2D_LIBRARY})
endif()

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

if(NOT(APPLE))
    target_link_libraries(gtaone stdc++fs)
endif()

target_include_directories(gtaone PUBLIC ${OPENAL_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})
target_link_libraries(gtaone glfw ${GLEW_LIBRARY} ${OPENAL_LIBRARY} ${OPENGL_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
