add_subdirectory(compresser)
add_subdirectory(request)
#add_subdirectory(evclient)
add_subdirectory(types)
add_subdirectory(utils)
add_subdirectory(storage)
add_subdirectory(scheduler)
add_subdirectory(graph)

find_package(Boost REQUIRED)

add_executable(replica)


IF(${SCHEDULER} STREQUAL "FREE")
    message ("Free Scheduling")
    target_compile_definitions(replica PUBLIC FREE)
ELSEIF(${SCHEDULER} STREQUAL "NON_STOP")
    message ("Non stop reparting")
    target_compile_definitions(replica PUBLIC NON_STOP)
ELSE()
    message ("Old method")
    target_compile_definitions(replica PUBLIC OLD)
ENDIF()

add_definitions( -DTRACK_LENGTH=${TRACK_LENGTH} )
add_definitions( -DQ_SIZE=${Q_SIZE} )
add_definitions( -DSCHEDULE_QUEUE_SIZE=${SCHEDULE_QUEUE_SIZE} )

target_sources(
    replica
        PRIVATE
            replica.cpp
)

target_link_libraries(
    replica
        PRIVATE
            Boost::boost
            CONAN_PKG::toml11
            request
            types
            utils
            scheduler
)