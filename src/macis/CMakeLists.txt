# MACIS Copyright (c) 2023, The Regents of the University of California,
# through Lawrence Berkeley National Laboratory (subject to receipt of
# any required approvals from the U.S. Dept. of Energy). All rights reserved.
#
# See LICENSE.txt for details

add_library( macis
  fcidump.cxx
  fock_matrices.cxx
  transform.cxx
  orbital_gradient.cxx
  casscf.cxx
  moller_plesset.cxx
  orbital_energies.cxx
  orbital_rotation_utilities.cxx
  orbital_hessian.cxx
  orbital_steps.cxx
)

target_include_directories( macis PUBLIC 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    #$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(macis PUBLIC cxx_std_17)

# Parallelism
find_package( MPI REQUIRED )
target_link_libraries( macis PUBLIC MPI::MPI_CXX )
if(MACIS_ENABLE_OPENMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries( macis PUBLIC OpenMP::OpenMP_CXX )
endif()

# Linear Algebra
find_package(Eigen3 CONFIG REQUIRED)
target_link_libraries(macis PUBLIC Eigen3::Eigen)

# Boost
if(MACIS_ENABLE_BOOST)
  find_package(Boost CONFIG REQUIRED)
  target_link_libraries(macis PUBLIC Boost::boost)
endif()

# Packaged Dependencies
target_link_libraries(macis PUBLIC lobpcgxx sparsexx)

# SPDLOG
include(macis-spdlog)
target_link_libraries(macis PUBLIC spdlog::spdlog)

# MDSPAN
include(macis-mdspan)
target_link_libraries(macis PUBLIC mdspan)

#find_package( Ceres REQUIRED )
#target_link_libraries( macis PUBLIC Ceres::ceres )


# Installation
include( GNUInstallDirs )
add_library(macis::macis ALIAS macis)
install( TARGETS macis lobpcgxx sparsexx ${MACIS_SPDLOG_EXPORT}
  EXPORT macis-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)



set_target_properties( macis PROPERTIES EXPORT_NAME macis )

# Export build tree
export(EXPORT macis-targets
      NAMESPACE macis::
      FILE "${PROJECT_BINARY_DIR}/macis-targets.cmake")

# Install staic headers
install( 
  DIRECTORY   ${PROJECT_SOURCE_DIR}/include
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  FILES_MATCHING PATTERN "*.hpp"
)

   
# Export target to script
set( INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/macis )
install( EXPORT macis-targets
  FILE         macis-targets.cmake
  NAMESPACE    macis::
  DESTINATION  ${INSTALL_CONFIGDIR}
)

# Create config-version.cmake file
include( CMakePackageConfigHelpers )
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/macis-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)


# Setup macis-config.cmake
configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/macis-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/macis-config.cmake
  INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

# Install CMake files
install( FILES
  ${CMAKE_CURRENT_BINARY_DIR}/macis-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/macis-config-version.cmake
  DESTINATION ${INSTALL_CONFIGDIR}
)

# Install Custom Find Modules
include( ${linalg-cmake-modules_SOURCE_DIR}/LinAlgModulesMacros.cmake )
install_linalg_modules( INSTALL_CONFIGDIR )
