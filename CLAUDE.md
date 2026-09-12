# picoperl

microperlをRP2350 Cortex-M33向けの最小Perlに

## 目標
- Perl 5.12.5 / Makefile.microベース
- NO_MATHOMS
- lib/なし
- NVはdoubleではなくfloatに
- doubleに依存しないよう言語側も修正する
- picoperlが使用するlibc関数を特定する
- 不要なCソース・OS機能・libc依存を段階的に削る
- ARM Linux/Thumbで中間検証後、arm-none-eabi / Cortex-M33へ移植
- 最終的にRAMFSとUARTだけで動作させる、QEMU libvirtで動くように

## 開発ルール
- 変更は小さく行い、毎回ビルドとテストを実行する
- 動かない機能をstubで誤魔化さず、依存理由を確認してから削る
- upstream全体を直接削らず、make-picoperl.shのFILESを編集して最小ツリーを作る
- FILESは明示列挙する。*.h一括コピーは禁止
- lib/はコピーしない
- 生成物(generate_uudmap, *.o, uudmap.h, bitcount.h, microperl)はソース一覧に入れない
- `git diff`を確認してから変更をまとめる
- まずx86_64 WSL/Debianで正常性を維持する

## 基本コマンド
```sh
./make-picoperl.sh
cd picoperl-5.12.5
./picoperl -e 'print 0.123, "OK\n"'
```

## Claude Codeへの指示
作業開始時にこのファイルと `make-picoperl.sh` を読むこと。
最初に `git status` を確認すること。
変更後は必ず `./make-picoperl.sh` を実行し、最小ツリー側でも再ビルドできることを確認すること。
gitでcommit,pushする前にmake cleanして不要なビルド生成物(.oなど)がコミットされないようにする。
