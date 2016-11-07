cmake_minimum_required(VERSION 2.8)

option(BUILD_FZ2EMB "Build Fritzing to embroidery convertor" OFF)

add_subdirectory(libembroidery)
include_directories( . )

add_executable(svg2emb
  svg2emb.cxx
)
target_link_libraries(svg2emb embroidery m)


if( BUILD_FZ2EMB )
  add_executable(fz2emb
    fz2emb.cxx
  )
  find_package(LibXml2 REQUIRED)
  include_directories(${LIBXML2_INCLUDE_DIR})
  target_link_libraries(fz2emb embroidery ${LIBXML2_LIBRARIES} m)
endif()
