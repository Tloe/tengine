set(HEADERS
    device.h
    image.h
    window.h
    swap_chain.h
    debug.h
    vulkan.h
    context.h)
set(SOURCES
    device.cpp
    image.cpp
    window.cpp
    swap_chain.cpp
    debug.cpp
    vulkan.cpp
    context.cpp)

target_sources(render PUBLIC ${HEADERS} PRIVATE ${SOURCES})
