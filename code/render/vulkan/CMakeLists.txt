set(HEADERS
    context.h)
set(SOURCES
    context.cpp)

target_sources(render PUBLIC ${HEADERS} PRIVATE ${SOURCES})
