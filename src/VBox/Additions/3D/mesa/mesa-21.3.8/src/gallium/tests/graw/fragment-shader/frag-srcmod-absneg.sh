FRAG

DCL IN[0], COLOR, LINEAR
DCL OUT[0], COLOR

DCL TEMP[0]

IMM FLT32 { -0.2, -0.3, -0.4, 0.0 }
IMM FLT32 { -1.0, -1.0, -1.0, -1.0 }

ADD TEMP[0], IN[0], IMM[0]
MOV TEMP[0], -|TEMP[0]|
MUL OUT[0], TEMP[0], IMM[1]

END
