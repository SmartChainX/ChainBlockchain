# all dependencies that are not directly included in the ChainX distribution are defined here

# for this to work, download the dependency via the cmake script in extdep or install them manually!



function(eth_show_dependency DEP NAME)

	get_property(DISPLAYED GLOBAL PROPERTY CHX_${DEP}_DISPLAYED)

	if (NOT DISPLAYED)

		set_property(GLOBAL PROPERTY ETH_${DEP}_DISPLAYED TRUE)

		message(STATUS "${NAME} headers: ${${DEP}_INCLUDE_DIRS}")

		message(STATUS "${NAME} lib   : ${${DEP}_LIBRARIES}")

		if (NOT("${${DEP}_DLLS}" STREQUAL ""))

			message(STATUS "${NAME} dll   : ${${DEP}_DLLS}")

		endif()

	endif()

endfunction()
