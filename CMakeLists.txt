
cmake_minimum_required ( VERSION 3.5.0 )
set(CMAKE_C_STANDARD 11)

project(KmtHsaTest)

SET(CMAKE_C_COMPILER g++)
ADD_COMPILE_OPTIONS(-std=c++11)
set ( KMTHSA_TARGET "KmtHsaTest.out" )

## Source files
#aux_source_directory(<dir> <variable>)
set (SRC 
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
add_executable ( ${KMTHSA_TARGET} ${SRC} )

target_link_libraries(${KMTHSA_TARGET}  ${CMAKE_CURRENT_SOURCE_DIR}/libamdhsacode/libelf/libelf.a)