cmake_minimum_required(VERSION 2.8.12)
#if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    FILE(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
    FILE(GLOB INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

    # set the variable "libraryName" with the name of the library
    # and make sure it's propagated to the parent scope
    # for using it in the libraryList
    set(libraryName CosmosDeviceI2C PARENT_SCOPE)

    add_library(CosmosDeviceI2C STATIC ${SOURCES} ${INCLUDES})
    target_link_libraries(CosmosDeviceI2C CosmosDeviceGeneral)

    install(TARGETS CosmosDeviceI2C DESTINATION lib)
    install(FILES ${INCLUDES} DESTINATION include/device/i2c)

    if(${DEPLOY_SOURCE_FILES_TO_REMOTE} MATCHES "yes")
        add_deployment_source_directory(${CMAKE_CURRENT_SOURCE_DIR} "src")
    endif()

    message("Build ${libraryName} from ${CMAKE_CURRENT_SOURCE_DIR}")
#endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")

