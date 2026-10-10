execute_process(
  COMMAND "${COMPILER}" "${SOURCE}" -o "${PRG}" -S "${ASM}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "memory variable example failed to compile: ${error}")
endif()

file(READ "${ASM}" assembly)
foreach(instruction IN ITEMS "sta $0400,x" "sta $D800,x" "sta $D020" "sta $D021" "sta $D015" "sta $D027,x" "lda $D012")
  string(FIND "${assembly}" "${instruction}" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "assembly is missing memory-mapped instruction '${instruction}'")
  endif()
endforeach()
file(SIZE "${PRG}" prg_size)
if(prg_size GREATER 1024)
  message(FATAL_ERROR "memory bindings unexpectedly expanded the PRG to ${prg_size} bytes")
endif()