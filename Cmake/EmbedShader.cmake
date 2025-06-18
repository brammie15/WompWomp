# EmbedShader.cmake
# Usage: -DINPUT=... -DOUTPUT=... -DVAR_NAME=...

file(READ "${INPUT}" BINARY_CONTENT HEX)

string(REPLACE ";" "" HEX_CONTENT "${BINARY_CONTENT}")

set(OUTPUT_FILE "// Auto-generated header from ${INPUT}\n")
set(OUTPUT_FILE "${OUTPUT_FILE}#pragma once\n\n")
set(OUTPUT_FILE "${OUTPUT_FILE}static const unsigned char ${VAR_NAME}[] = {")

string(LENGTH "${HEX_CONTENT}" LEN)
math(EXPR NUM_BYTES "${LEN} / 2")

math(EXPR LAST_BYTE "${NUM_BYTES} - 1")
foreach(i RANGE 0 ${LAST_BYTE} 1)
    math(EXPR OFFSET "2 * ${i}")
    string(SUBSTRING "${HEX_CONTENT}" ${OFFSET} 2 BYTE)
    set(OUTPUT_FILE "${OUTPUT_FILE} 0x${BYTE},")
endforeach()

set(OUTPUT_FILE "${OUTPUT_FILE} };\n")
set(OUTPUT_FILE "${OUTPUT_FILE}static const unsigned int ${VAR_NAME}_len = sizeof(${VAR_NAME});\n")

file(WRITE "${OUTPUT}" "${OUTPUT_FILE}")
