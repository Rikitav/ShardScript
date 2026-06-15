file(READ "${IN_FILE}" HEX_CONTENT HEX)

string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1, " ARRAY_CONTENT "${HEX_CONTENT}")

string(LENGTH "${HEX_CONTENT}" HEX_LENGTH)
math(EXPR BYTE_LENGTH "${HEX_LENGTH} / 2")

set(HEADER_TEXT "unsigned char ${VAR_NAME}[] = { ${ARRAY_CONTENT} };\n")
string(APPEND HEADER_TEXT "unsigned int ${VAR_NAME}_len = ${BYTE_LENGTH};\n")

file(WRITE "${OUT_HEADER}" "${HEADER_TEXT}")