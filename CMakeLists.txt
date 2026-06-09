#----------------------------------------------------------------------------
# RADiCAL GEANT4 simulation
#----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.16...3.30)
project(radical)

# Locate GEANT4 with UI + visualization drivers.
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(radical radical.cc ${sources} ${headers})
target_link_libraries(radical ${Geant4_LIBRARIES})

# Copy run macros next to the executable so they can be executed in-place.
set(RADICAL_MACROS
  init_vis.mac vis.mac run_beam.mac single_module.mac
  array3x3.mac hex_module.mac enhanced_module.mac energy_scan.mac)
foreach(_m ${RADICAL_MACROS})
  configure_file(${PROJECT_SOURCE_DIR}/macros/${_m}
                 ${PROJECT_BINARY_DIR}/macros/${_m} COPYONLY)
endforeach()

install(TARGETS radical DESTINATION bin)
