; haribote-ipl
; TAB=4

    ORG       0x7c00            ; このプログラムがどこに読み込まれるのか

; 以下は標準的な FAT12 フォーマットフロッピーディスクのための記述

    JMP       entry
    DB        0x90
    DB        "HARIBOTE"        ; ブートセクタの名前を自由に書いて良い(8バイト)
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
    DB        "HARIBOTEOS "     ; ディスクの名前(11バイト)
    DB        "FAT12   "        ; フォーマットの名前
    RESB      18                ; とりあえず18バイトあけておく

; プログラム本体

entry:
    MOV       AX,0              ; レジスタ初期化
    MOV       SS,AX
    MOV       SP,0x7c00
    MOV       DS,AX

; ディスクを読み込む

    MOV       AX,0x0820
    MOV       ES,AX
    MOV       CH,0              ; シリンダ0
    MOV       DH,0              ; ヘッド0
    MOV       CL,2              ; セクタ2
readloop:
    MOV       SI,0              ; 失敗回数を数えるレジスタ
retry:
    MOV       AH,0x02           ; AX-0x02 : ディスク読み込み
    MOV       AL,1              ; 1セクタ
    MOV       BX,0
    MOV       DL,0x00           ; A ドライブ
    INT       0x13              ; ディスク BIOS 呼び出し
    JNC       next               ; エラーが起きなければ next へ
    ADD       SI,1              ; SI に1を足す
    CMP       SI,5              ; SI を5と比較
    JAE       error             ; ST >= 5 だったら error he
    MOV       AH,0x00
    MOV       DL,0x00           ; A ドライブ
    INT       0x13              ; ドライブのリセット
    JMP       retry
next:
    MOV       AX,ES             ; アドレスを 0x200 進める
    ADD       AX,0x0020
    MOV       ES,AX             ; ADD ES,0x20 という命令がないのでこうしている
    ADD       CL,1              ; CL に1足す
    CMP       CL,18             ; CL と18を比較
    JBE       readloop          ; CL <= 18 だったら readloop へ

; 読み終わったけど取り合えすやることないので寝る

fin:
    HLT                         ; 何かあるまで CPU を停止させる
    JMP       fin               ; 無限ループ

error:
    MOV       AX,0
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
msg:
    DB        0x0a, 0x0a        ; 改行を2つ
    DB        "hello, world"
    DB        0x0a              ; 改行
    DB        0

    RESB      0x1fe-($-$$)      ; 0x001fe までを 0x00 で埋める命令

    DB        0x55, 0xaa
