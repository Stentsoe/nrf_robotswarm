# Point to NCS root directory.
set(NRF_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "NCS root directory")

# We need to cache the build type so that it is not out of sync when CONF_FILE
# is cached.
set(CONF_FILE_BUILD_TYPE ${CONF_FILE_BUILD_TYPE} CACHE INTERNAL "The build type")

# include(cmake/extensions.cmake)
# include(cmake/version.cmake)
# include(cmake/multi_image.cmake)

zephyr_include_directories(include)


add_subdirectory(subsys)
# add_subdirectory(lib)