/*
 * env.h - libcのgetenv(3)/putenv(3)に依存しない、picoperl自前の
 * 環境変数ストア。
 *
 * 設計方針(性能よりシンプルさ):
 *   - "NAME=VALUE"文字列をエントリ丸ごと保持する単方向リスト
 *     (ファイル数を決め打ちしないramfs/vfsと同じ発想)
 *   - putenv(3)と同じ契約: 渡した文字列の所有権を受け取り、以後
 *     freeしない("freeしないハッシュに格納する"というTODOの方針通り)。
 *     上書き時も古いエントリをfreeしない(本物のputenv(3)がしばしば
 *     leakするのと同じ割り切り。Cortex-M33の実行寿命では問題にならない)
 *   - 削除(unsetenv相当)は実装しない。このビルドは`d_unsetenv='undef'`
 *     でPerl側もunsetenvを呼ばないため不要
 *
 * picoperl_getenv/picoperl_putenvはlibc-pico2/stdlib.hから
 * getenv/putenvの代わりに呼ばれる(picoperl本体・romperl両方が
 * 同じpicoperl-5.12.5/env.o経由でこれを使う。romfs/ramfs/vfsとは
 * 異なりromperl専用の強い実装への差し替えは無い、常にこの実装のみ)。
 */
#ifndef PICOPERL_ENV_H
#define PICOPERL_ENV_H

/* 起動時に一度だけ呼ぶ。envp(main()の第3引数、無ければNULLでよい)を
 * 複製して初期値として取り込む。perl.c自身がPerlEnv_getenv()経由で
 * PERL_DESTRUCT_LEVEL等の起動時環境変数を参照するため、これが無いと
 * それらが常に「未設定」になってしまう。
 */
void env_init(char **envp);

/* 全エントリを解放して空の状態に戻す(テストのリセット用)。 */
void env_reset(void);

const char *picoperl_getenv(const char *name);

/* stringは"NAME=VALUE"形式。所有権を受け取り以後freeしない
 * (putenv(3)と同じ契約)。'='が無ければ失敗(-1)。 */
int picoperl_putenv(char *string);

#endif /* PICOPERL_ENV_H */
