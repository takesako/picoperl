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
 * - 圧縮なし。ファイルデータはエントリ順に隙間なく連結されるだけ。
 * - read-only。書き込み・削除・追記は一切サポートしない。
 *
 * イメージ全体のレイアウト:
 *   [romfs_header]
 *   [romfs_entry] * header.file_count
 *   [ファイルデータ] (エントリ順に連結、パディング無し)
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

#endif /* PICOPERL_ROMFS_H */
