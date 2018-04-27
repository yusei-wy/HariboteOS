; hello-os
; TAB=4

    ORG       0x7c00            ; このプログラムがどこに読み込まれるのか

; 以下は標準的な FAT12 フォーマットフロッピーディスクのための記述

    JMP       entry
    DB        0x90
    DB        "HELLOIPL"        ; ブートセクタの名前を自由に書いて良い(8バイト)
    DW        512               ; 1セクタの大きさ(512にしなければならない)
    DB        1                 ; クラスタの大きさ
    DW        1                 ; FAT がどこから始まるか
    DB        2                 ; FAT の個数
    DW        224               ; ルートディレクトリ領域の大きさ
    DW        2280              ; このドライブの大きさ
    DB        0xf0              ; メディアのタイプ
    DW        9                 ; FAT 領域の長さ
    DW        18                ; 1トラックにいくつのセクタがあるか
    DW        2                 ; ヘッドの数
    DD        0                 ; パーティションを使ってないのでここは必ず0
    DD        2880              ; このドライブの大きさをもう一度
    DB        0,0,0x29          ; よくわからないけどこの値にしておくといいらしい
    DD        0xffffffff        ; 多分ボリュームシリアル番号
    DB        "HELLO-OS   "     ; ディスクの名前(11バイト)
    DB        "FAT12   "        ; フォーマットの名前
    RESB      18                ; とりあえず18バイトあけておく

; プログラム本体

entry:
    MOV       AX,0              ; レジスタ初期化
    MOV       SS,AX
    MOV       SP,0x7c00
    MOV       DS,AX
    MOV       ES,AX

    MOV       SI,msg
putloop:
    MOV       AL,[SI]
    ADD       SI,1              ; SI に1を足す
    CMP       AL,0
    JE        fin
    MOV       AH,0x0e           ; 1文字表示ファンクション
    MOV       BX,15             ; カラーコード
    INT       0x10              ; ビデオ BIOS 呼び出し
    JMP       putloop           
fin:
    HLT                         ; 何かあるまで CPU を停止させる
    JMP       fin               ; 無限ループ
msg:
    DB        0x0a, 0x0a        ; 改行を2つ
    DB        "hello, world"
    DB        0x0a              ; 改行
    DB        0

    RESB      0x1fe-($-$$)      ; 0x001fe までを 0x00 で埋める命令

    DB        0x55, 0xaa
