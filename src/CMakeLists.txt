set(PARSI_C_SOURCES parsi-c.cpp)

add_library(parsi-c)
add_library(${PROJECT_NAME}::parsi-c ALIAS parsi-c)

target_sources(parsi-c PRIVATE ${PARSI_C_SOURCES})
target_link_libraries(parsi-c PUBLIC ${PROJECT_NAME}::${PROJECT_NAME})
