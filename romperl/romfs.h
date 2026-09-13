/*
 * picoperl向け ROMFS フォーマット仕様。
 *
 * 設計方針: 互換性より実装の単純さを優先する自作フォーマット。
 * - ディレクトリ木は持たない。エントリ名はフルパス文字列("lib/feature.pm"の
 *   ように先頭'/'なし)で、検索は文字列完全一致のみ(readdir的なものは無い)。
 * - 全ての整数はホストのネイティブエンディアンでそのまま格納する。
 *   x86_64 (開発ホスト) も Cortex-M33 (RP2350, 実行ターゲット) も
 *   どちらもリトルエンディアンのため、バイトスワップ処理を持たない。
 *   他アーキテクチャに移植する場合はこの前提を見直すこと。
 * - 圧縮なし。ファイルデータはエントリ順に連結されるが、各ファイルの直後に
 *   ';' を1byteだけ挟む(entry.sizeには含めない、通常のread/statからは
 *   見えない隠しパディング)。これは pp_require を直接ROMFSに繋ぐ経路
 *   (romfs_data_for_compile)が、Perl 5.12.5のlex_start()のゼロコピー
 *   分岐(入力の最後のバイトが';'ならSVをコピーしない)を満たすための
 *   センチネル。
 * - read-only。書き込み・削除・追記は一切サポートしない。
 *
 * イメージ全体のレイアウト:
 *   [romfs_header]
 *   [romfs_entry] * header.file_count
 *   [ファイルデータ + ';'センチネル1byte] をエントリ順に連結
 */
#ifndef PICOPERL_ROMFS_H
#define PICOPERL_ROMFS_H

#include <stdint.h>

#define ROMFS_MAGIC "RFS1"      /* 4バイト、NUL終端なし */
#define ROMFS_NAME_MAX 32       /* NUL込みの最大長。実質31文字まで */

struct romfs_header {
    char     magic[4];         /* ROMFS_MAGIC と一致すること */
    uint32_t file_count;       /* エントリ数 */
    uint32_t total_size;       /* イメージ全体のバイト数 (ヘッダ+エントリ+データ) */
    uint32_t reserved;         /* 0固定。将来のフォーマットバージョン用に予約 */
};                              /* 16 bytes */

struct romfs_entry {
    char     name[ROMFS_NAME_MAX]; /* NUL終端パス。ROMFS_NAME_MAX未満は0埋め */
    uint32_t offset;               /* イメージ先頭からファイルデータまでのオフセット */
    uint32_t size;                  /* ファイルサイズ (バイト) */
};                              /* 40 bytes */

/*
 * 最小 open/read/seek/close/stat API。
 *
 * ファイルディスクリプタはROMFS_MAX_OPEN個の固定配列から割り当てる
 * (動的確保なし)。パスはromfs_entry.nameとの完全一致でのみ検索する。
 * romfs_init() を呼ぶ前に他の romfs_* 関数を呼んではいけない。
 */
#define ROMFS_MAX_OPEN 8

#define ROMFS_SEEK_SET 0
#define ROMFS_SEEK_CUR 1
#define ROMFS_SEEK_END 2

struct romfs_stat {
    uint32_t size;
};

/* image/size は .romfs セクションに埋め込まれたイメージの先頭とバイト数。
 * magicが不正な場合は0を、正常なら1を返す。 */
int  romfs_init(const unsigned char *image, unsigned long size);

/* 見つからない/空きfdが無い場合は-1 */
int  romfs_open(const char *path);
/* 読めたバイト数 (0はEOF)。fdが不正なら-1 */
long romfs_read(int fd, void *buf, unsigned long len);
/* 新しい位置。whenceが不正/fdが不正なら-1。範囲外は[0,size]にクランプする */
long romfs_lseek(int fd, long offset, int whence);
/* 常に0。fdが不正でも黙って無視する (close(2)相当の緩さ) */
int  romfs_close(int fd);
/* 見つかれば0、無ければ-1 */
int  romfs_stat(const char *path, struct romfs_stat *out);

/*
 * ゼロコピー読み出し用。ROMFSイメージ内のファイルデータそのものへの
 * ポインタ(mallocしたコピーではない)を返す。呼び出し側はこの領域を
 * 書き換えてはいけない(read-only、ROMFSイメージが生きている間だけ有効)。
 * 見つからなければNULLを返す。
 */
const void *romfs_data(const char *path, unsigned long *out_size);

/*
 * pp_require を直接ROMFSに繋ぐ経路専用。romfs_data() と同じポインタを
 * 返すが、*out_size にはファイル直後の ';' センチネル1byteを含めた
 * entry->size+1 を入れる(Perl 5.12.5 の lex_start() が「入力の最後の
 * バイトが';'ならコピーせずそのSVを使う」という分岐を通るようにするため)。
 * 見つからなければNULLを返す。
 */
const void *romfs_data_for_compile(const char *path, unsigned long *out_size);

#endif /* PICOPERL_ROMFS_H */
