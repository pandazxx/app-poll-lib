# Always safe to include this
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Create the library
zephyr_library_named(__MODULE_NAME__)

zephyr_library_sources(
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/__MODULE_NAME__.c
)

# Optionally only build/link if config is enabled
if(CONFIG___MODULE_NAME_UPPER__)
  target_link_libraries(app PRIVATE __MODULE_NAME__)
endif()

