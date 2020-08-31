
cmake_minimum_required ( VERSION 3.5.0 )
set(CMAKE_C_STANDARD 11)

project(KmtHsaStudy)

SET(CMAKE_C_COMPILER g++)
ADD_COMPILE_OPTIONS(-std=c++11)
set ( PROJ_TARGET "KmtHsaStudy.out" )

## Source files
#aux_source_directory(<dir> <variable>)
set (PROJ_SRC 
		"src/kmthsa.cpp"
		"src/memory.cpp"
		"src/queue.cpp"
		"src/sdma.cpp"
		"src/event.cpp"
		"src/executable.cpp"
		"src/aql.cpp"
		"src/main.cpp"
		"libamdhsacode/amd_elf_image.cpp"
		"libamdhsacode/amd_hsa_code.cpp"
		"libamdhsacode/amd_hsa_code_util.cpp"
	)
	
include_directories ( ${CMAKE_CURRENT_SOURCE_DIR}/include )
include_directories ( ${CMAKE_CURRENT_SOURCE_DIR}/libamdhsacode )
include_directories ( ${CMAKE_CURRENT_SOURCE_DIR}/src )

#add_library (libelf.a)
add_executable ( ${PROJ_TARGET} ${PROJ_SRC} )
target_link_libraries(${PROJ_TARGET}  ${CMAKE_CURRENT_SOURCE_DIR}/libamdhsacode/libelf/libelf.a)
