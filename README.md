# krkrrel-ng

`krkrrel-ng`は[吉里吉里Releaser](https://krkrz.github.io/krkr2doc/kr2doc/contents/Releaser.html)の移植版であり、新しいツールチェーンでコンパイルして最新のプラットフォームで実行できます。  

## ダウンロード

以下のビルドは、Github Actionsによって最新のソースコードから自動的に構築されます。

* [Win32ビルド](https://github.com/krkrsdl2/krkrrel-ng/releases/download/latest/krkrrel-win32.zip)
* [macOSビルド](https://github.com/krkrsdl2/krkrrel-ng/releases/download/latest/krkrrel-macos.zip)
* [Ubuntuビルド](https://github.com/krkrsdl2/krkrrel-ng/releases/download/latest/krkrrel-ubuntu.zip)

## 使用法

Releaser は以下のコマンドラインオプションを受け付けます。

* フォルダ名: ターゲットのフォルダを指定します。
* `-go`: このオプションは、元のReleaserとの互換性のためにスキップされます。
* `-nowriterpf`: 終了時にプロジェクトディレクトリに`default.rpf`を書き込まないでください。
* `-out <filename>`: 出力ファイルのパスを`<filename>`に設定します。
* `-rpf <filename>`: アーカイブ操作を開始する前に、`<filename>`からプロファイルをロードします。

For example, specify:
```bash
/path/to/krkrrel /path/to/project/directory -out /path/to/data.xp3 -nowriterpf -go
```

## クローニング

リポジトリのクローンを作成するには、ターミナルで次のコマンドを使用してください：

```bash
git clone --recursive https://github.com/krkrsdl2/krkrrel-ng
```
プロジェクトがGitサブモジュールを使用するため、上記のコマンドを正しく使用しない場合、ソースファイルが欠落します。

## ビルディング

このプロジェクトは、CMakeビルドシステムを使用してビルドできます。  
CMakeビルドシステムの詳細については、次の場所をご覧ください： https://cmake.org/  

## オリジナルプロジェクト

このプロジェクトのコードは、次のプロジェクトに基づいています：
* [吉里吉里Z](https://github.com/krkrz/krkrz) dev_multi_platform ブランチ
* [krdevui](https://github.com/krkrz/krdevui)

## IRCチャンネル

吉里吉里SDL2プロジェクトのメンバーは、[freenodeの#krkrsdl2チャンネル](https://webchat.freenode.net/?channel=#krkrsdl2)で見つけることができます。

## ライセンス

このコードは、修正された3条項のBSDライセンスに基づいています。 詳細については、`LICENSE`をお読みください。  
このプロジェクトには、サードパーティのコンポーネントが含まれています。 詳細については、各コンポーネントのライセンスファイルを参照してください。  
