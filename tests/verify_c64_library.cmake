execute_process(
  COMMAND "${COMPILER}" "${SOURCE}" -o "${PRG}" -S "${ASM}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "C64 library example failed to compile: ${error}")
endif()

file(READ "${ASM}" assembly)
foreach(fragment IN ITEMS "jsr f_screen_put_row0" "jsr f_sprite_position" "jsr f_sid_set_frequency" "f_key:" "jsr $FFE4")
  string(FIND "${assembly}" "${fragment}" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "assembly is missing C library fragment '${fragment}'")
  endif()
endforeach()