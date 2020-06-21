project(stl2ngc)

cmake_minimum_required(VERSION 3.9)

set(CMAKE_VERBOSE_MAKEFILE OFF)

if (CMAKE_VERSION VERSION_LESS "3.1")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
  endif ()
else ()
  set (CMAKE_CXX_STANDARD 11)
endif ()



if (CMAKE_BUILD_TOOL MATCHES "make")
    add_definitions(-Wall -Werror -Wno-deprecated -pedantic-errors)
endif (CMAKE_BUILD_TOOL MATCHES "make")

find_package( OpenMP REQUIRED )
IF (OPENMP_FOUND)
    MESSAGE(STATUS "found OpenMP, compiling with flags: " ${OpenMP_CXX_FLAGS} )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
ENDIF(OPENMP_FOUND)

find_library(OCL_LIBRARY 
            NAMES ocl.dll
            PATHS E:/msys64/mingw64/lib/opencamlib
			      E:/windows-linux/vcpkg/installed/x64-windows/lib
            DOC "The opencamlib library"
)
MESSAGE(STATUS "OCL_LIBRARY is now: " ${OCL_LIBRARY})

add_executable(stl2ngc
   
	stl2ngc.cpp
    
)

target_link_libraries(stl2ngc ${OCL_LIBRARY})


install (TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)



#install(FILES ${stl2ngc} DESTINATION bin)
