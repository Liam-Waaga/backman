
include_directories(PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


add_subdirectory(log)
add_subdirectory(parser)
add_subdirectory(target)


add_executable(
  backman
  main.cpp
  utils.cpp
)

target_link_libraries(
  backman
  PRIVATE
  log
  parser
  target
)
