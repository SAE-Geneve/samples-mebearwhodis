cmake_minimum_required (VERSION 3.18)
project(GPR5300)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)


file(GLOB_RECURSE SHADER_FILES
        "data/*.vert"
        "data/*.frag"
        "data/*.comp"
        "data/*.geom"
        )
if(MSVC)
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
        set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
    else()
        set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
    endif()
elseif(UNIX)
    set(GLSL_VALIDATOR "glslangValidator")
endif()
foreach(SHADER ${SHADER_FILES})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    get_filename_component(PATH_NAME ${SHADER} DIRECTORY)
    get_filename_component(EXTENSION ${SHADER} EXT)
    file(RELATIVE_PATH PATH_NAME "${CMAKE_CURRENT_SOURCE_DIR}" ${PATH_NAME})
    #MESSAGE("Data PATH: ${PATH_NAME} NAME: ${FILE_NAME}")
    set(SHADER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PATH_NAME}/${FILE_NAME}")
    #MESSAGE("Data OUT PATH: ${DATA_OUTPUT}")
    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E copy
            ${SHADER}
            ${SHADER_OUTPUT}
            COMMAND ${GLSL_VALIDATOR}  ${SHADER}
            DEPENDS ${SHADER})
    list(APPEND SCRIPT_OUTPUT_FILES ${SHADER_OUTPUT})
endforeach(SHADER)

add_custom_target(shader_target
        DEPENDS ${SCRIPT_OUTPUT_FILES}
)

file(GLOB_RECURSE DATA_FILES
            "data/*.json"
            "data/*.png"
            "data/*.jpg"
            "data/*.bmp"
            "data/*.hdr"
            "data/*.obj"
            "data/*.mtl"
            )
    foreach(DATA ${DATA_FILES})
        get_filename_component(FILE_NAME ${DATA} NAME)
        get_filename_component(PATH_NAME ${DATA} DIRECTORY)
        get_filename_component(EXTENSION ${DATA} EXT)
        file(RELATIVE_PATH PATH_NAME "${CMAKE_CURRENT_SOURCE_DIR}" ${PATH_NAME})
        #MESSAGE("Data PATH: ${PATH_NAME} NAME: ${FILE_NAME}")
        set(DATA_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PATH_NAME}/${FILE_NAME}")
        #MESSAGE("Data OUT PATH: ${DATA_OUTPUT}")
        add_custom_command(
                OUTPUT ${DATA_OUTPUT}
                COMMAND ${CMAKE_COMMAND} -E copy
                ${DATA}
                ${DATA_OUTPUT}
                DEPENDS ${DATA})
        list(APPEND Data_OUTPUT_FILES ${DATA_OUTPUT})
    endforeach(DATA)


    add_custom_target(
            data_target
            DEPENDS ${Data_OUTPUT_FILES}
    )


file(GLOB_RECURSE COMMON_FILES src/*.cpp include/*.h)
add_library(Common STATIC ${COMMON_FILES} ${SHADER_FILES})
target_include_directories(Common PUBLIC include/  ${Stb_INCLUDE_DIR})
target_link_libraries(Common PUBLIC GLEW::GLEW glm::glm SDL2::SDL2 SDL2::SDL2main imgui::imgui)
set_target_properties(Common PROPERTIES UNITY_BUILD ON)
add_dependencies(Common shader_target data_target)

if(MSVC)
    target_compile_definitions(Common PUBLIC "_USE_MATH_DEFINES" WIN32_LEAN_AND_MEAN)
    target_compile_options(Common PUBLIC /arch:AVX2 /Oi /GL /fp:fast)
    target_link_options(Common PUBLIC /LTCG)
else()

endif()

file(GLOB MAIN_FILES main/*.cpp)
foreach(MAIN_FILE ${MAIN_FILES})
    get_filename_component(MAIN_NAME ${MAIN_FILE} NAME_WE)
    
    add_executable(${MAIN_NAME} ${MAIN_FILE})
    target_link_libraries(${MAIN_NAME} PUBLIC Common) 
endforeach()