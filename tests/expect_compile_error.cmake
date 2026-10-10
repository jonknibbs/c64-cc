execute_process(
  COMMAND "${COMPILER}" "${SOURCE}" -o "${OUTPUT}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
)
if(result EQUAL 0)
  message(FATAL_ERROR "expected compilation to fail for ${SOURCE}")
endif()
if(NOT "${error}" MATCHES "${EXPECTED}")
  message(FATAL_ERROR "expected error matching '${EXPECTED}', got: ${error}")
endif()