aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")

add_executable(chainx ${SRC_LIST} ${HEADERS})

find_package(chainx)
find_package(Web3)

target_include_directories(eth PRIVATE ..)
target_link_libraries(eth ${chainx_EVM_LIBRARIES})
target_link_libraries(eth ${Web3_WEB3JSONRPC_LIBRARIES})
target_link_libraries(eth ${Web3_WEBTHREE_LIBRARIES})

if (EVMJIT)
	# Do we need include paths here?
	target_link_libraries(chainx ${Eth_EVMJIT_LIBRARIES})
	eth_copy_dlls(EVMJIT_DLLS)
endif()

if (UNIX AND NOT APPLE)
	target_link_libraries(eth pthread)
endif()

install(TARGETS DESTINATION bin)
