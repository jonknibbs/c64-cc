execute_process(
  COMMAND "${COMPILER}" "${SOURCE}" -o "${PRG}" -S "${ASM}"
  RESULT_VARIABLE result
  ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "comment example failed to compile: ${error}")
endif()

file(READ "${ASM}" assembly)
foreach(expected
    "; // Line comment retained in the assembly listing."
    "; /* Block comment"
    ";      * with more than one line."
    "; // Trailing comment retained too."
    "; function: main"
    "f_main:")
  string(FIND "${assembly}" "${expected}" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "expected ASM output to contain: ${expected}")
  endif()
endforeach()