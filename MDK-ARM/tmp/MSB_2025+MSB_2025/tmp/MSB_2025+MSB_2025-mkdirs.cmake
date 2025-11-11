# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/MengshengCup2025/Control/MSB_2025/MDK-ARM/tmp/MSB_2025+MSB_2025")
  file(MAKE_DIRECTORY "D:/MengshengCup2025/Control/MSB_2025/MDK-ARM/tmp/MSB_2025+MSB_2025")
endif()
file(MAKE_DIRECTORY
  "D:/MengshengCup2025/Control/MSB_2025/MDK-ARM/tmp/1"
  "D:/MengshengCup2025/Control/MSB_2025/MDK-ARM/tmp/MSB_2025+MSB_2025"
  "D:/MengshengCup2025/Control/MSB_2025/MDK-ARM/tmp/MSB_2025+MSB_2025/tmp"
  "D:/MengshengCup2025/Control/MSB_2025/MDK-ARM/tmp/MSB_2025+MSB_2025/src/MSB_2025+MSB_2025-stamp"
  "D:/MengshengCup2025/Control/MSB_2025/MDK-ARM/tmp/MSB_2025+MSB_2025/src"
  "D:/MengshengCup2025/Control/MSB_2025/MDK-ARM/tmp/MSB_2025+MSB_2025/src/MSB_2025+MSB_2025-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/MengshengCup2025/Control/MSB_2025/MDK-ARM/tmp/MSB_2025+MSB_2025/src/MSB_2025+MSB_2025-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/MengshengCup2025/Control/MSB_2025/MDK-ARM/tmp/MSB_2025+MSB_2025/src/MSB_2025+MSB_2025-stamp${cfgdir}") # cfgdir has leading slash
endif()
