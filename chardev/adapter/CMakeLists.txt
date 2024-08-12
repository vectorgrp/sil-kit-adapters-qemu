add_library(
    ChardevSocketToPubSubAdapter STATIC
    ChardevSocketToPubSubAdapter.cpp)
    
target_include_directories(ChardevSocketToPubSubAdapter 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
           ${CMAKE_SOURCE_DIR})
target_link_libraries(ChardevSocketToPubSubAdapter
    PRIVATE
        asio
        SilKit::SilKit
)
set_target_properties(ChardevSocketToPubSubAdapter 
	PROPERTIES
	#ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY}>
)

if(MINGW)
	target_compile_definitions(ChardevSocketToPubSubAdapter INTERFACE _WIN32_WINNT=0x0601)
	target_link_libraries(ChardevSocketToPubSubAdapter PUBLIC -lwsock32 -lws2_32) #windows socket/ wsa
endif()
