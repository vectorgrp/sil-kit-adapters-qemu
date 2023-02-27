
add_library(EthSocketToEthControllerAdapter
    "EthSocketToEthControllerAdapter.cpp"
)

target_include_directories(EthSocketToEthControllerAdapter 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
           ${CMAKE_SOURCE_DIR})

target_link_libraries(EthSocketToEthControllerAdapter
    PRIVATE
        Utility
        asio
        SilKit::SilKit
)
set_target_properties(EthSocketToEthControllerAdapter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY}>
)
