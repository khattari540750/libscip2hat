libscip2hat
============
北陽製レーザー測域センサ " URG " を利用するためのライブラリ


動作環境
--------
-  OSX
-  Linux
-  Microsoft Windows
    with MinGW/Cygwin
         and pthreads_win32 (http://sources.redhat.com/pthreads-win32/)


ビルド・インストール方法
----------------------
 このプロジェクトは、以下の手順にて、cmake を利用してビルド、インストールします。
 あらかじめ cmake をインストールしておいてください。

    $ git clone http://gitbucket.team-lab.local/git/hattori/libscip2hat.git
    $ cd libscip2hat
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ sudo make install


使い方
------
サンプルプログラムを使って動作テストを行います。このプログラムでは、URGの正面方向における、物体までの距離が表示されます。

1. まず、URGをPCに接続し、電源を入れます。
2. 先ほどダウンロードしたパッケージのディレクトリから、サンプルの保存されているディレクトリへ移動します。
   `$ cd libscip2hat/sample`
3. サンプルプログラムを実行します。
   `$./test-ms /dev/cu.usbmodem`
   終了する際は、
   `control + c`
ボタンを押すことで停止できます。
このとき、/dev/cu.usbmodem とは、URGが接続されているポート情報ファイルを示しており、環境によってその文字列は変化します。
URGをポートから抜き差しした際、/dev/内で現れたり消えたりするものが対応したポート情報ファイルなので、それを引数として与えます。
