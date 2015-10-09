# <http://www.procon.gr.jp/>

## 厳密解

``` sh
    $ ./generator.py --board-area 0.9 --board-complexity 8 --block-size 8 --block-complexity 4 --board-size $s --block-number $i
```

で作成したケースで試験した。

`s = 16`, `i = 8, 10, 12, 14` では1秒も要せず即座に停止する。容易にブロックを使いきれるためだと思われる。  
`s = 16`, `i = 16` 以降は極端に遅い。1時間近くやっても停止しなかった。

( commit e6cf2a8d9a4b6b140f8d9e956731c5e85a60e041 時 )

## 補助ツール

SDL2.0,SDL2_image,SDL2_gfx,SDL2_ttfが必要です

``` sh
    $ cd GUIapp
    $ make
    $ ./GUIapp.elf <game num>
```
![画面](GUIapp/doc/img/image1.png "画面")
-Tabキー: 裏返す
-Spaceキー: 90度回転

## linkとか

-   <http://www.procon.gr.jp/uploads/procon26/Apply26.pdf>
-   <http://en.wikipedia.org/wiki/Exact_cover>
