
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
        sil-kit-adapters-common
)
set_target_properties(EthSocketToEthControllerAdapter 
	PROPERTIES
    #ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
	RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY}>
)
