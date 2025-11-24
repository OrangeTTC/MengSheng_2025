# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/Orange/Desktop/for_fu/for_fu/MDK-ARM/tmp/for_fu+for_fu")
  file(MAKE_DIRECTORY "C:/Users/Orange/Desktop/for_fu/for_fu/MDK-ARM/tmp/for_fu+for_fu")
endif()
file(MAKE_DIRECTORY
  "C:/Users/Orange/Desktop/for_fu/for_fu/MDK-ARM/tmp/1"
  "C:/Users/Orange/Desktop/for_fu/for_fu/MDK-ARM/tmp/for_fu+for_fu"
  "C:/Users/Orange/Desktop/for_fu/for_fu/MDK-ARM/tmp/for_fu+for_fu/tmp"
  "C:/Users/Orange/Desktop/for_fu/for_fu/MDK-ARM/tmp/for_fu+for_fu/src/for_fu+for_fu-stamp"
  "C:/Users/Orange/Desktop/for_fu/for_fu/MDK-ARM/tmp/for_fu+for_fu/src"
  "C:/Users/Orange/Desktop/for_fu/for_fu/MDK-ARM/tmp/for_fu+for_fu/src/for_fu+for_fu-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Orange/Desktop/for_fu/for_fu/MDK-ARM/tmp/for_fu+for_fu/src/for_fu+for_fu-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Orange/Desktop/for_fu/for_fu/MDK-ARM/tmp/for_fu+for_fu/src/for_fu+for_fu-stamp${cfgdir}") # cfgdir has leading slash
endif()
