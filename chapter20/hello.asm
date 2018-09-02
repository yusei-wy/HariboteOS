; アプリはセグメント'1003*8', OS はセグメント'2*8'の中で動いている
MOV     AL,'h'
INT     0x40
MOV     AL,'e'
INT     0x40
MOV     AL,'l'
INT     0x40
MOV     AL,'l'
INT     0x40
MOV     AL,'o'
INT     0x40
RETF
