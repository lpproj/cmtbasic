CMTBASIC for N88-BASIC(86)
==========================

PC-9801/9821シリーズに内蔵されているROM BASICにはカセットテープ（CMT）に対するセーブ・ロード機能が存在しますが、世の大部分のPC-98シリーズにはカセットインターフェースがないため宝の持ち腐れになっています。
本プログラムはシステムのCMT BIOSによる入出力をFATフロッピーディスクへの入出力にリダイレクトして、ROM BASICでもプログラムのセーブとロードを可能にしてみよう、というものです。


動作要件
--------

* N88-BASIC(86)を内蔵しているノーマルモードのPC-98シリーズ（ハイレゾ未対応）
* 基本メモリ256Kバイト以上（640Kバイト状態でのみ動作確認）
* V30以上のCPU（ブートローダーが80186以上必須のため）


（おおまかな）用法
------------------

CMTBASIC起動用フロッピーをPC-98に挿入し、FDDから起動するとROM内蔵のN88-BASIC(86)が起動するはずです。
（例の"How many files(0-15)?"のメッセージが出ない場合は本体がROM BASICを使用しない設定になっている可能性があります）

カセットに対するセーブとロードは起動フロッピーに対するファイルの入出力となります。  
セーブ開始時には画面上部にメニューが出るので、保存するファイル名を入力して（MS-DOSと同じ8.3形式。BASICのSAVE命令で指定する名前とは別）リターンキーを押すとセーブが開始されます。（同名ファイルが存在する場合は上書きしてよいか確認メッセージが出ます）  
ロード開始時にも画面上部にファイルブラウザ的なメニューが表示されるので、カーソルの上下キーで選んでリターンキーを押して決定します。  
また、セーブ／ロード開始時でなくても、CTRL+HELPキーでメニューを呼び出すことができます。


ソースのビルド
--------------

ソースはOpenWatcom 2.0betaでビルドを行っています。（OpenWatcom 1.9でもおそらくビルド可能）  
ブートローダー付きのディスクイメージを作成するには、本パッケージのほかにPC-98対応版のBootProgが必要です。(https://github.com/lpproj/BootProg/)

```
wmake -h -f Makefile.wc bareimg
```

ライセンス
----------

本プログラムは以下のライブラリを使用しています。ライセンスに関しては各ライブラリのドキュメントを参照してください。

* [FatFS](http://elm-chan.org/fsw/ff/00index_e.html) - Copyright (C) 2006-2021, ChaN, all right reserved.
* [xprintf](http://elm-chan.org/fsw/strf/xprintf.html) - Copyright (C) 2021, ChaN, all right reserved.

それ以外の自作コードに関しては[Unlicense](https://unlicense.org/)に準じます。

