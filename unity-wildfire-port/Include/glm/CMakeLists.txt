file(GLOB ROOT_SOURCE *.cpp)
file(GLOB ROOT_INLINE *.inl)
file(GLOB ROOT_HEADER *.hpp)
file(GLOB ROOT_TEXT ../*.txt)
file(GLOB ROOT_MD ../*.md)
file(GLOB ROOT_NAT ../util/glm.natvis)

file(GLOB_RECURSE CORE_SOURCE ./detail/*.cpp)
file(GLOB_RECURSE CORE_INLINE ./detail/*.inl)
file(GLOB_RECURSE CORE_HEADER ./detail/*.hpp)

file(GLOB_RECURSE EXT_SOURCE ./ext/*.cpp)
file(GLOB_RECURSE EXT_INLINE ./ext/*.inl)
file(GLOB_RECURSE EXT_HEADER ./ext/*.hpp)

file(GLOB_RECURSE GTC_SOURCE ./gtc/*.cpp)
file(GLOB_RECURSE GTC_INLINE ./gtc/*.inl)
file(GLOB_RECURSE GTC_HEADER ./gtc/*.hpp)

file(GLOB_RECURSE GTX_SOURCE ./gtx/*.cpp)
file(GLOB_RECURSE GTX_INLINE ./gtx/*.inl)
file(GLOB_RECURSE GTX_HEADER ./gtx/*.hpp)

file(GLOB_RECURSE SIMD_SOURCE ./simd/*.cpp)
file(GLOB_RECURSE SIMD_INLINE ./simd/*.inl)
file(GLOB_RECURSE SIMD_HEADER ./simd/*.h)

source_group("Text Files" FILES ${ROOT_TEXT} ${ROOT_MD})
source_group("Core Files" FILES ${CORE_SOURCE})
source_group("Core Files" FILES ${CORE_INLINE})
source_group("Core Files" FILES ${CORE_HEADER})
source_group("EXT Files" FILES ${EXT_SOURCE})
source_group("EXT Files" FILES ${EXT_INLINE})
source_group("EXT Files" FILES ${EXT_HEADER})
source_group("GTC Files" FILES ${GTC_SOURCE})
source_group("GTC Files" FILES ${GTC_INLINE})
source_group("GTC Files" FILES ${GTC_HEADER})
source_group("GTX Files" FILES ${GTX_SOURCE})
source_group("GTX Files" FILES ${GTX_INLINE})
source_group("GTX Files" FILES ${GTX_HEADER})
source_group("SIMD Files" FILES ${SIMD_SOURCE})
source_group("SIMD Files" FILES ${SIMD_INLINE})
source_group("SIMD Files" FILES ${SIMD_HEADER})

add_library(glm-header-only INTERFACE)
add_library(glm::glm-header-only ALIAS glm-header-only)

target_include_directories(glm-header-only INTERFACE
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

if (GLM_BUILD_LIBRARY)
	add_library(glm
		${ROOT_TEXT}      ${ROOT_MD}        ${ROOT_NAT}
		${ROOT_SOURCE}    ${ROOT_INLINE}    ${ROOT_HEADER}
		${CORE_SOURCE}    ${CORE_INLINE}    ${CORE_HEADER}
		${EXT_SOURCE}     ${EXT_INLINE}     ${EXT_HEADER}
		${GTC_SOURCE}     ${GTC_INLINE}     ${GTC_HEADER}
		${GTX_SOURCE}     ${GTX_INLINE}     ${GTX_HEADER}
		${SIMD_SOURCE}    ${SIMD_INLINE}    ${SIMD_HEADER}
	)
	add_library(glm::glm ALIAS glm)
	target_link_libraries(glm PUBLIC glm-header-only)
else()
	add_library(glm INTERFACE)
	add_library(glm::glm ALIAS glm)
	target_link_libraries(glm INTERFACE glm-header-only)
endif()

#include Eigen
include_directories(${root_path}/ext/eigen)

#include hearder and source files in /src
file(GLOB_RECURSE src_cpp ${root_path}/src/*.cpp)
file(GLOB_RECURSE src_h ${root_path}/src/*.h)
list(APPEND src_files ${src_cpp} ${src_h})
source_group("src" FILES ${src_cpp} ${src_h})
include_directories(${root_path}/src)
	
#include hearder and source files in /proj/a*
file(GLOB_RECURSE proj_cpp ${proj_src_path}/*.cpp)
file(GLOB_RECURSE proj_h ${proj_src_path}/*.h)
list(APPEND src_files ${proj_cpp} ${proj_h})
source_group("proj" FILES ${proj_cpp} ${proj_h})
include_directories(${proj_path}/src)

#include glm
include_directories(${root_path}/ext/glm)

#include freeglut and glew
if(WIN32)
	set(freeglut_src_path ${root_path}/ext/freeglut/include)
	set(freeglut_lib_path ${root_path}/ext/freeglut/lib/x64)
	include_directories(${freeglut_src_path})

	set(glut_libs debug ${freeglut_lib_path}/freeglutd.lib optimized ${freeglut_lib_path}/freeglut.lib)
	message(STATUS ${glut_libs})
	list(APPEND lib_files ${glut_libs})
		
elseif(APPLE)  #freeglut and glew are installed on macos by "brew install freeglut glew"
	find_package(GLUT REQUIRED)
	if (GLUT_FOUND)
    		include_directories(${GLUT_INCLUDE_DIRS})
    		link_libraries(${GLUT_LIBRARIES})
	endif()

	find_package(OpenGL REQUIRED)
    	include_directories(${OpenGL_INCLUDE_DIRS})
    	link_directories(${OpenGL_LIBRARY_DIRS})
    	add_definitions(${OpenGL_DEFINITIONS})

	list(APPEND lib_files ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

elseif(UNIX) #freeglut and glew are installed on linux by "sudo apt-get install freeglut3-dev"
	set(GCC_COVERAGE_COMPILE_FLAGS "${GCC_COVERAGE_COMPILE_FLAGS} -lGL -lglut -lGLU -ldl")
	set(GCC_COVERAGE_LINK_FLAGS "${GCC_COVERAGE_LINK_FLAGS} -lGL -lglut -lGLU -ldl")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
	set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
endif(WIN32)

#include tiny_obj_loader
file(GLOB_RECURSE tiny_obj_cpp ${root_path}/ext/tiny_obj_loader/*.cpp)
file(GLOB_RECURSE tiny_obj_h ${root_path}/ext/tiny_obj_loader/*.h)
list(APPEND src_files ${tiny_obj_cpp} ${tiny_obj_h})
source_group("tiny_obj_loader" FILES ${tiny_obj_cpp} ${tiny_obj_h})
include_directories(${root_path}/ext/tiny_obj_loader)

#include stb
file(GLOB_RECURSE stb_cpp ${root_path}/ext/stb/*.cpp)
file(GLOB_RECURSE stb_h ${root_path}/ext/stb/*.h)
list(APPEND src_files ${stb_cpp} ${stb_h})
source_group("stb" FILES ${stb_cpp} ${stb_h})
include_directories(${root_path}/ext/stb)

#include tiny_gltf
file(GLOB_RECURSE tiny_gltf_cpp ${root_path}/ext/tiny_gltf/*.cpp)
file(GLOB_RECURSE tiny_gltf_h ${root_path}/ext/tiny_gltf/*.h ${root_path}/ext/tiny_gltf/*.hpp)
list(APPEND src_files ${tiny_gltf_cpp} ${tiny_gltf_h})
source_group("tiny_gltf" FILES ${tiny_gltf_cpp} ${tiny_gltf_h})
include_directories(${root_path}/ext/tiny_gltf)
