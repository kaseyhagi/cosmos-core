cmake_minimum_required(VERSION 2.8.12)

#include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

message("COSMOS_AGENST_LIBS: ${COSMOS_LIBS}")

foreach(file ${files})

    get_filename_component(bfile ${file} NAME_WE)

    add_executable(${bfile} ${bfile}.cpp)

    target_link_libraries(${bfile} ${COSMOS_LIBS})

    install(TARGETS ${bfile} DESTINATION bin)

    if(${DEPLOY_FILES_TO_REMOTE} MATCHES "yes")
        add_deployment_binary(${bfile} "bin")
        add_deployment_source(${bfile}.cpp "src")
    endif()

endforeach()

if(${BUILD_AGENTS_TYPE} MATCHES "ALL")
    add_subdirectory(ground-station)
endif()

if(${BUILD_AGENTS_TYPE} MATCHES "ALL")
    add_subdirectory(other)
endif()


#add_executable(agent_cpu agent_cpu.cpp)
#target_link_libraries(agent_cpu ${COSMOS_LIBS})
