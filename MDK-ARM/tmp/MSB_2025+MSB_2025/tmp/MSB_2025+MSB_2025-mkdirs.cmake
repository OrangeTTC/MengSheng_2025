# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/Orange/Desktop/MengSheng_2025-PID_Control/MengSheng_2025-PID_Control/MDK-ARM/tmp/MSB_2025+MSB_2025")
  file(MAKE_DIRECTORY "C:/Users/Orange/Desktop/MengSheng_2025-PID_Control/MengSheng_2025-PID_Control/MDK-ARM/tmp/MSB_2025+MSB_2025")
endif()
file(MAKE_DIRECTORY
  "C:/Users/Orange/Desktop/MengSheng_2025-PID_Control/MengSheng_2025-PID_Control/MDK-ARM/tmp/1"
  "C:/Users/Orange/Desktop/MengSheng_2025-PID_Control/MengSheng_2025-PID_Control/MDK-ARM/tmp/MSB_2025+MSB_2025"
  "C:/Users/Orange/Desktop/MengSheng_2025-PID_Control/MengSheng_2025-PID_Control/MDK-ARM/tmp/MSB_2025+MSB_2025/tmp"
  "C:/Users/Orange/Desktop/MengSheng_2025-PID_Control/MengSheng_2025-PID_Control/MDK-ARM/tmp/MSB_2025+MSB_2025/src/MSB_2025+MSB_2025-stamp"
  "C:/Users/Orange/Desktop/MengSheng_2025-PID_Control/MengSheng_2025-PID_Control/MDK-ARM/tmp/MSB_2025+MSB_2025/src"
  "C:/Users/Orange/Desktop/MengSheng_2025-PID_Control/MengSheng_2025-PID_Control/MDK-ARM/tmp/MSB_2025+MSB_2025/src/MSB_2025+MSB_2025-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Orange/Desktop/MengSheng_2025-PID_Control/MengSheng_2025-PID_Control/MDK-ARM/tmp/MSB_2025+MSB_2025/src/MSB_2025+MSB_2025-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Orange/Desktop/MengSheng_2025-PID_Control/MengSheng_2025-PID_Control/MDK-ARM/tmp/MSB_2025+MSB_2025/src/MSB_2025+MSB_2025-stamp${cfgdir}") # cfgdir has leading slash
endif()
