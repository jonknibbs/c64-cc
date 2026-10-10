execute_process(
  COMMAND "${COMPILER}" "${SOURCE}" -o "${PRG}" -S "${ASM}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "placement example failed to compile: ${error}")
endif()

file(READ "${ASM}" assembly)
foreach(origin IN ITEMS "*= $1000" "*= $2000")
  string(FIND "${assembly}" "${origin}" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "assembly listing is missing origin '${origin}'")
  endif()
endforeach()
file(SIZE "${PRG}" prg_size)
if(prg_size LESS 6149)
  message(FATAL_ERROR "placed PRG should contain padding through $2003, got ${prg_size} bytes")
endif()