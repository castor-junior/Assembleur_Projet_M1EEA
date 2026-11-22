; Test de l'assembleur

MOV R0, R1
LDI R1, 0x01

boucle:
    SUB R0, R1
    JNZ boucle

    ST R0, 0x80
    HALT
