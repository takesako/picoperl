/*
 * vfs.h - romfs(../romperl/romfs.h、読み取り専用・Flash埋め込み)と
 * ramfs(../ramfs/ramfs.h、書き込み可能・RAM上)を1本のfopen系APIに
 * まとめる薄いディスパッチ層。
 *
 * 設計方針(ユーザーとの合意事項):
 *   - romfsとramfsの実装自体は変更しない。vfsはどちらを呼ぶかを
 *     振り分けるだけの層
 *   - 書き込みを伴うopen("w"/"a"/末尾に'+'を含む全モード)は常に
 *     ramfsだけを対象にする。romfsには一切触れない
 *     (romfsは常にread-only、という前提を絶対に崩さないため)
 *   - 読み取り専用open("r")は、まずramfsを見て、無ければromfsに
 *     フォールバックする
 *   - 名前がromfsとramfsの両方に存在する場合の優先順位はramfs勝ち。
 *     これは特別なshadow/whiteout処理をしなくても「読み取りは
 *     ramfs優先」というルールだけで自然に実現される
 *     (romfs上の"lib/feature.pm"と同名でramfsに書き込めば、以後の
 *     読み取りはramfs側の内容が見える)
 *   - vfs_removeはramfsだけを対象にする(romfs上のファイルは削除
 *     できない=read-onlyという前提を崩さないため)。ramfsとromfsの
 *     両方に同名ファイルがある状態でramfs側だけをremoveすると、
 *     以後の読み取りはromfs側の内容に「戻る」(whiteoutは実装して
 *     いないため)。この程度の衝突時の挙動は未定義/保証しないという
 *     割り切りで、実用上romfsに入れているのはビルド時埋め込みの
 *     固定モジュール("lib/feature.pm"等)だけなので実害は無い想定
 *
 * vfs_init()を呼ぶ前提として、romfs側は別途 romfs_init(image, size) が
 * 呼ばれていること(vfsはromfsイメージの生存期間を管理しない)。
 * vfs_init()はramfsの書き込み内容だけをリセットする。
 */
#ifndef VFS_H
#define VFS_H

typedef struct vfs_FILE vfs_FILE;

void vfs_init(void);

/* モード文字列の解釈は ramfs_fopen() に準じる ("r"/"w"/"a"/"r+"/"w+"/"a+"、
 * 末尾の'b'は無視)。書き込みを伴うモードは常にramfs、読み取り専用モード
 * ("r")はramfs優先・romfsフォールバックで解決する。
 */
vfs_FILE *vfs_fopen(const char *name, const char *mode);

unsigned long vfs_fread(void *ptr, unsigned long size, unsigned long nmemb, vfs_FILE *fp);
unsigned long vfs_fwrite(const void *ptr, unsigned long size, unsigned long nmemb, vfs_FILE *fp);

#define VFS_SEEK_SET 0
#define VFS_SEEK_CUR 1
#define VFS_SEEK_END 2

int  vfs_fseek(vfs_FILE *fp, long offset, int whence);
long vfs_ftell(vfs_FILE *fp);
int  vfs_feof(vfs_FILE *fp);
int  vfs_fclose(vfs_FILE *fp);

/* ramfs側にのみ作用する(romfs上のファイルは削除できない)。
 * 見つからなければ-1。 */
int vfs_remove(const char *name);

struct vfs_stat {
    unsigned long size;
};
/* ramfs優先・romfsフォールバックでstatする。見つからなければ-1。 */
int vfs_stat(const char *name, struct vfs_stat *st);

#endif /* VFS_H */
