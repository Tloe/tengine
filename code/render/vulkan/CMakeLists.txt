set(HEADERS
    debug.h
    vulkan.h
    context.h)
set(SOURCES
    debug.cpp
    vulkan.cpp
    context.cpp)

target_sources(render PUBLIC ${HEADERS} PRIVATE ${SOURCES})
