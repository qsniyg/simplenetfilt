set(BASE_SOURCES
  main.c
)

set(LIB64_DIR lib64)
execute_process(COMMAND readlink -f /usr/lib64 OUTPUT_VARIABLE LIB64_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
get_filename_component(LIB64_LIBDIR_BASENAME ${LIB64_LIBDIR} NAME)
if(LIB64_LIBDIR_BASENAME STREQUAL "lib")
  set(LIB64_DIR lib)
endif()

set(LIB32_DIR lib32)
execute_process(COMMAND readlink -f /usr/li32 OUTPUT_VARIABLE LIB32_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
get_filename_component(LIB32_LIBDIR_BASENAME ${LIB32_LIBDIR} NAME)
if(LIB32_LIBDIR_BASENAME STREQUAL "lib")
  set(LIB32_DIR lib)
endif()

add_library(simplenetfilt_lib64 SHARED ${BASE_SOURCES})
target_link_libraries(simplenetfilt_lib64 dl)
set_target_properties(simplenetfilt_lib64 PROPERTIES OUTPUT_NAME "simplenetfilt")
set_target_properties(simplenetfilt_lib64 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${LIB64_DIR}")

add_library(simplenetfilt_lib32 SHARED ${BASE_SOURCES})
target_link_libraries(simplenetfilt_lib32 dl)
set_target_properties(simplenetfilt_lib32 PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
set_target_properties(simplenetfilt_lib32 PROPERTIES OUTPUT_NAME "simplenetfilt")
set_target_properties(simplenetfilt_lib32 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${LIB32_DIR}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../simplenetfilt" "${PROJECT_BINARY_DIR}/bin/simplenetfilt" COPYONLY USE_SOURCE_PERMISSIONS)

install(TARGETS simplenetfilt_lib32 DESTINATION "${CMAKE_INSTALL_PREFIX}/${LIB32_DIR}")
install(TARGETS simplenetfilt_lib64 DESTINATION "${CMAKE_INSTALL_PREFIX}/${LIB64_DIR}")
install(FILES "${PROJECT_BINARY_DIR}/bin/simplenetfilt" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" PERMISSIONS WORLD_READ WORLD_EXECUTE)

#set(SOURCES
#  ${BASE_SOURCES}
#  main.cpp
#  )

#add_executable(winevfs ${SOURCES})
#target_link_libraries(winevfs stdc++fs dl Threads::Threads)
#target_compile_definitions(winevfs PRIVATE SERVER_BUILD=1)
#set_target_properties(winevfs PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
