aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_executable(chainx ${SRC_LIST} ${HEADERS})

find_package(chainx)
find_package(Web3)

target_include_directories(chainx PRIVATE ..)
target_link_libraries(chainx ${chainx_EVM_LIBRARIES})
target_link_libraries(chainx ${Web3_WEB3JSONRPC_LIBRARIES})
target_link_libraries(chainx ${Web3_WEBTHREE_LIBRARIES})

if (EVMJIT)
	# Do we need include paths here?
	target_link_libraries(chainx ${chainx_EVMJIT_LIBRARIES})
	chainx_copy_dlls(EVMJIT_DLLS)
endif()

if (UNIX AND NOT APPLE)
	target_link_libraries(chainx pthread)
endif()

install(TARGETS DESTINATION bin)
