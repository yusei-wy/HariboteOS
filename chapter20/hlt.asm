BITS 32
    ; アプリはセグメント'1003*8', OS はセグメント'2*8'の中で動いている
    MOV     AL,'h'
    CALL    2*8:0xca2
    MOV     AL,'e'
    CALL    2*8:0xca2
    MOV     AL,'l'
    CALL    2*8:0xca2
    MOV     AL,'l'
    CALL    2*8:0xca2
    MOV     AL,'o'
    CALL    2*8:0xca2
    RETF
