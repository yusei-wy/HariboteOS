BITS 32
    MOV     AL,'A'
    CALL    2*8:0xca2   ; アプリはセグメント'1003*8', OS はセグメント'2*8'の中で動いている
fin:
    HLT
    JMP     fin
