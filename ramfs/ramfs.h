/*
 * ramfs.h - シンプルな書き込み可能インメモリファイルシステム。
 *
 * romfs(romperl/romfs.h)がFlash埋め込みの読み取り専用イメージだったのに
 * 対して、ramfsは実行時にファイルの新規作成・更新・削除ができる、RAM上
 * だけで完結するファイルシステム。将来的にはlibcの fopen 系関数の裏側
 * (PerlIOやlibcのstdioシム)から呼ばれる想定なので、API は独自の
 * open/read/write ではなく fopen/fread/fwrite/fseek/ftell/fclose/remove
 * に対応させた名前と引数構成にしてある。ただしPOSIX/ISO Cとの互換性は
 * 目指さない(値を合わせているのは分かりやすさのためだけで、
 * 例えばfopenのモード文字列は最低限しか解釈しない)。
 *
 * 設計方針(性能よりシンプルさ):
 *   - フラットな名前空間(ディレクトリ階層は無い。romfsと同じ)
 *   - ファイル数・サイズの上限は決め打ちしない
 *   - 事前の大きなメモリ確保はしない。書き込みの都度、必要な分だけ
 *     malloc/realloc する(バッファの倍々確保のような最適化はしない)
 *   - スレッドセーフではない(picoperlはシングルスレッド前提)
 */
#ifndef RAMFS_H
#define RAMFS_H

#define RAMFS_NAME_MAX 32

typedef struct ramfs_FILE ramfs_FILE;

/* 全ファイルを解放してファイルシステムを空の状態に戻す。
 * プログラム起動時、またはテストの初期化に使う。
 */
void ramfs_init(void);

/*
 * mode は fopen(3) の最低限のサブセットのみ解釈する:
 *   "r"  既存ファイルを読み取り用に開く。無ければ失敗(NULL)。
 *   "w"  書き込み用に開く。無ければ新規作成、あれば内容を空にする。
 *   "a"  追記用に開く。無ければ新規作成。書き込みは常に末尾に行われる。
 *   "r+" 既存ファイルを読み書き両用で開く。無ければ失敗。
 *   "w+" "w"と同じだが読み取りもできる。
 *   "a+" "a"と同じだが読み取りもできる。
 * 末尾の 'b' (バイナリ) は無視してよい(テキスト/バイナリの区別が
 * そもそも無いため常にバイナリ相当)。上記以外のmodeはNULLを返す。
 */
ramfs_FILE *ramfs_fopen(const char *name, const char *mode);

/* 戻り値はfread(3)と同じ: 読み取れた「要素数」(size*nmembバイトでは
 * ない)。EOFやエラーではsize/nmembの実際の積で割った値になる。
 */
unsigned long ramfs_fread(void *ptr, unsigned long size, unsigned long nmemb, ramfs_FILE *fp);
unsigned long ramfs_fwrite(const void *ptr, unsigned long size, unsigned long nmemb, ramfs_FILE *fp);

#define RAMFS_SEEK_SET 0
#define RAMFS_SEEK_CUR 1
#define RAMFS_SEEK_END 2

int  ramfs_fseek(ramfs_FILE *fp, long offset, int whence);
long ramfs_ftell(ramfs_FILE *fp);
int  ramfs_feof(ramfs_FILE *fp);
int  ramfs_fclose(ramfs_FILE *fp);

/* remove(3)相当。開いているfdが無くても呼べる。成功で0、無ければ-1。 */
int ramfs_remove(const char *name);

struct ramfs_stat {
    unsigned long size;
};
/* 成功で0、無ければ-1。 */
int ramfs_stat(const char *name, struct ramfs_stat *st);

#endif /* RAMFS_H */
