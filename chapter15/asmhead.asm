; haribote-os boot nasm
; TAB=4

VBEMODE EQU     0x105           ; 1024 x 768 x 8bit カラー
; (画面モード一覧)
;   0x100 : 640 x 400 x 8bit color
;   0x101 : 640 x 480 x 8bit color
;   0x103 : 800 x 600 x 8bit color
;   0x105 : 1024 x 768 x 8bit color
;   0x107 : 1280 x 1024 x 8bit color

BOTPAK  EQU     0x00280000      ; bootpack のロード先
DSKCAC  EQU     0x00100000      ; ディスクキャッシュの場所
DSKCAC0 EQU     0x00008000      ; ディスクキャッシュの場所(リアルモード)

; BOOT_INFO 関係
CYLS    EQU     0x0ff0          ; ブートセクタが設定する
LEDS    EQU     0x0ff1
VMODE   EQU     0x0ff2          ; 色数に関する情報, 何ビットカラーか？
SCRNX   EQU     0x0ff4          ; 解像度の X(screen x)
SCRNY   EQU     0x0ff6          ; 解像度の Y(screen y)
VRAM    EQU     0x0ff8          ; グラフィックバッファの開始番地

        ORG     0xc200          ; このプログラムがどこに読み込まれるのか

; VBE 存在確認
        
        MOV     AX,0x9000
        MOV     ES,AX
        MOV     DI,0
        MOV     AX,0x4f00
        INT     0x10
        CMP     AX,0x004f
        JNE     scrn320

; VBE のバージョンチェック
        
        MOV     AX,[ES:DI+4]
        CMP     AX,0x0200
        JB      scrn320          ; if (AX < 0x0200) goto scrn320

; 画面モード情報を得る

        MOV     CX,VBEMODE
        MOV     AX,0x4f01
        INT     0x10
        CMP     AX,0x004f
        JNE     scrn320

; 画面モード情報の確認

        CMP     BYTE [ES:DI+0x19],8
        JNE     scrn320
        CMP     BYTE [ES:DI+0x1b],4
        JNE     scrn320
        MOV     AX,[ES:DI+0x00]
        AND     AX,0x0080
        JZ      scrn320          ; モード属性の bit7 が0だったので諦める

; 画面モードの切り替え

        MOV     BX,VBEMODE+0x4000
        MOV     AX,0x4f02
        INT     0x10
        MOV     BYTE [VMODE],8 ; 描画モードをメモする(C言語が参照する)
        MOV     AX,[ES:DI+0x12]
        MOV     [SCRNX],AX
        MOV     AX,[ES:DI+0x14]
        MOV     [SCRNY],AX
        MOV     EAX,[ES:DI+0x28]
        MOV     [VRAM],EAX
        JMP     keystatus

scrn320:
        MOV     AL,0x13         ; VGA グラフィックス, 320x200x8bit color
        MOV     AH,0x00
        INT     0x10
        MOV     BYTE [VMODE],8  ; 画面モードをメモる(C言語が参照する)
        MOV     WORD [SCRNX],320
        MOV     WORD [SCRNY],200
        MOV     DWORD [VRAM],0x000a0000

; キーボードの LED 状態を BIOS に教えてもらう

keystatus:

        MOV     AH,0x02
        INT     0x16           ; keyboard BIOS
        MOV     [LEDS],AL

; PIC が一切の割り込みを受けないようにする
; AT 互換機の仕様では PIC の初期化をするなら,
; こいつを CLI 前にやっておかないとたまにハングアップする
; PIC の初期化はあとでやる

        MOV        AL,0xff
        OUT        0x21,AL
        NOP                       ; OUT 命令を連続させるとうまく行かない機種があるらしいので
        OUT        0xa1,AL

        CLI                   ; さらに CPU レベルでも割り込み禁止

; CPU から 1MB 以上のメモリにアクセスできるように A20GATE を設定

        CALL    waitkbdout
        MOV        AL,0xd1
        OUT        0x64,AL
        CALL    waitkbdout
        MOV        AL,0xdf        ; enable A20
        OUT        0x60,AL
        CALL    waitkbdout

; プロテクトモード移行

; nasm だと使えない
;[INSTRSET "i486p"]            ; 486の命令まで使いたいという記述

        LGDT    [GDTR0]        ; 暫定 GDT を設定
        MOV     EAX,CR0
        AND     EAX,0x7fffffff ; bit31 を0にする(ページング禁止のため)
        OR      EAX,0x00000001 ; bit0 を1にする(プロテクトモード移行のため)
        MOV     CR0,EAX
        JMP     pipelineflash
pipelineflash:
        MOV     AX,1*8         ; 読み書き可能セグメント 32bit
        MOV     DS,AX
        MOV     ES,AX
        MOV     FS,AX
        MOV     GS,AX
        MOV     SS,AX

; bootpack の転送

        MOV     ESI,bootpack   ; 転送元
        MOV     EDI,BOTPAK     ; 転送先
        MOV     ECX,512*1024/4
        CALL    memcpy

; ついでにディスクデータも本来の位置へ転送

; まずはブートセクタから

        MOV     ESI,0x7c00     ; 転送元
        MOV     EDI,DSKCAC     ; 転送先
        MOV     ECX,512/4
        CALL    memcpy

; 残り全部

        MOV     ESI,DSKCAC0+512 ; 転送元
        MOV     EDI,DSKCAC+512  ; 転送先
        MOV     ECX,0
        MOV     CL,BYTE [CYLS]
        IMUL    ECX,512*18*2/4  ; シリンダ数からバイト数/4に変換
        SUB     ECX,512/4       ; IPL の分だけ差し引く
        CALL    memcpy

; asmhead でしなければいけないことは全部し終わったので
; あとは bootpack に任せる

; bootpack の起動

        MOV     EBX,BOTPAK
        MOV     ECX,[EBX+16]
        ADD     ECX,3            ; ECX += 3;
        SHR     ECX,2            ; ECX /= 4;
        JZ      skip             ; 転送するべきものが内
        MOV     ESI,[EBX+20]     ; 転送元
        ADD     ESI,EBX
        MOV     EDI,[EBX+12]     ; 転送先
        CALL    memcpy
skip:
        MOV     ESP,[EBX+12]     ; スタック初期値
        JMP     DWORD 2*8:0x0000001b

waitkbdout:
        IN      AL,0x64
        AND     AL,0x02
        IN      AL,0x60           ; 空読み(受信バッファが悪さをしないように)
        JNZ     waitkbdout        ; AND の結果が0でなければ waitkbdout へ
        RET

memcpy:
        MOV     EAX,[ESI]
        ADD     ESI,4
        MOV     [EDI],EAX
        ADD     EDI,4
        SUB     ECX,1
        JNZ     memcpy            ; 引き算した結果が0でなければ memcpy へ
        RET
; memcpy はアドレスサイズプリフィックスを入れ忘れなければ, ストリング命令もかける

        ALIGNB  16
GDT0:
        RESB    8                  ; 塗るセレクタ
        DW      0xffff,0x0000,0x9200,0x00cf  ; 読み書き可能セグメント 32bit
        DW      0xffff,0x0000,0x9a28,0x0047  ; 事項可能セグメント 32bit(bootpack 用)

        DW      0
GDTR0:
        DW      8*3-1
        DD      GDT0

        ALIGNB  16
bootpack: