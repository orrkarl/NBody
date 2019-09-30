find_program(GLSLC glslc)

function(add_shader TARGET SHADER DESTINATION)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${GLSLC} -o ${CMAKE_CURRENT_BINARY_DIR}/${DESTINATION} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        VERBATIM
    )
endfunction()