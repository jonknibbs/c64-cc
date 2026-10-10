execute_process(
  COMMAND "${COMPILER}" "${SOURCE}" -o "${PRG}" -S "${ASM}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "optimization example failed to compile: ${error}")
endif()

file(READ "${ASM}" assembly)
string(FIND "${assembly}" "lda #$05" folded_constant)
if(folded_constant EQUAL -1)
  message(FATAL_ERROR "expected the folded value to be emitted as lda #$05")
endif()
string(FIND "${assembly}" "  adc " addition_instruction)
if(NOT addition_instruction EQUAL -1)
  message(FATAL_ERROR "constant addition was not folded")
endif()