# 第6回 レポート記入ガイド

第5回のALUに続き、今回は値を保持する部品を扱います。呼出し前に何を覚えていて、何を出力し、呼出し後に何が残ったかを、自分の図・コード・実行記録で説明してください。[空欄Word雛形](ex06-report-template.docx) ／ [教材の仕様とコマンド](../README.md#commands)

## 記録を始める前に

学籍番号、氏名、実施日、OSと版、CPU、使ったホスト用C17コンパイラの実パスと版、GNU Make版、教材commitを記入します。`make check`の表示、`make --version`、`git rev-parse HEAD`で確認できます。確認済み環境とTA実機確認待ちの環境は[教材README](../README.md#environment)を参照してください。

課題Aは`tasks/register_task.c`、Bは`tasks/memory_task.c`の指定TODO内を編集します。補助関数は指定HELPERS内へ置きます。`tests/student_cases.c`は全体を編集できます。境界コメント、公開署名、境界外、配布部品と公式テストを維持してください。AとBは独立して検証できます。

`Signal`はbool、`Word`は32本の論理信号を表す構造体で、bit[0]がLSBです。`sizeof(Word)`は4とは限りません。if・for・配列・添字計算・整数とWordの変換を使えます。状態は呼出し元から渡された構造体に保持します。本演習のタイミング、語の配置と別配列は教材モデルの契約です。命令実行・PC・制御器との接続は第7回で扱います。

## A-1　部品の動作

`register_run`の書込み有効時と無効時を、呼出し前の値・読出し出力・呼出し後の値で説明します。`decoder5`の5入力と全32出力、`mux32`の5制御と選ばれたWordを、自分の入力と実結果で対応付けます。番号は添字0がLSBです。呼出しごとに出力全ビットを定義できていることを、異なる入力や前回の出力から確かめてください。

## A-2　内部接続（図1）

RegisterFileの内部接続図を描きます。書込み番号→Decoder5→32個のRegisterの書込み制御、書込みデータ、各Registerの旧値→2個のMUX32、2つの読出し番号と出力を示します。x0の読出し・書込みをどこで扱うかも示してください。図1を本文から参照し、配線名と自分の関数・変数・配列・コード位置を対応付けます。自分の入力を1つたどり、選択された経路を説明します。数値テストは出力と状態、SOURCEは編集境界を検査します。指定部品の接続はソースと図から確認します。

## A-3　状態推移（表1）

`make test-a`、`make example-register`、`make example-register-file`と自分のケースを使います。観察ドライバーは`examples/observe_register.c`と`examples/observe_register_file.c`です。表1をRegisterとRegisterFileに分け、以下を同じ行または同じIDで対応付けます。

| 記録 | 書く内容 |
|---|---|
| 入力と順序 | ケースID、呼出し順、初期化、書込み許可・データ、RFの両読出し番号と書込み番号 |
| 状態と出力 | 呼出し前の値、仕様から求めた期待出力、実出力（RFは各ポート）、呼出し後の値、非選択状態 |
| 判定と説明 | 合否、選定理由、不一致なら最初に異なる段階・修正または残る疑問 |

同じ番号への読書きでも両ポートは更新前の値を読みます。書込み無効の場合、異なる2本を読む場合、x0を読み書きする場合も説明します。x0の規則はRegisterFileに適用されます。

## B-1　アドレス対応（図2）

InstMemoryとDataMemoryはそれぞれ1024語、1語32ビットでモデル容量4096バイトです。基点は命令`0x00400000`、データ`0x10010000`。図2に両基点、4バイト刻み、バイト差分、語添字、最後の有効語（基点＋4092、添字1023）を示します。観察した具体的なアドレスを図と本文に記し、自分の検査・変換・配列のコードに結び付けます。有効アクセスでは配布関数が下限→整列→上限を検査してから配列を参照します。非整列を拒否するのはこの教材の規則です。

## B-2　メモリの観察（表2）

`make test-b`と`make example-memory`を実行します。`examples/observe_memory.c`と独自ケースの実結果から、InstMemoryの読出しと全状態保持、DataMemoryの複数語の更新・読戻しを別々に記録します。

| W,R（mem_write, mem_read） | 読出し出力 | 観察する状態・検査順 |
|---|---|---|
| 0,0 | 0 | アドレス未使用・無検査。全語を保持 |
| 0,1 | 指定語の旧値 | アドレス検査後に読み、全語を保持 |
| 1,0 | 0 | 検査後、指定1語だけ更新 |
| 1,1 | 通常復帰しない | アドレス検査より先にCONTROL（生終了11） |

表2にはケースID・呼出し順、対象、W/R、入力アドレスと書込み値、添字（未使用ならその旨）、前状態、期待値、実値、後状態と非選択語、合否と理由を記入します。アドレスエラーは生終了10。通常復帰しないケースは隔離された公式ログの理由と生終了値を記録し、返ってこない値・状態は「通常復帰なし」と記します。スライドの表は期待値です。自分で観察した結果を別欄に記録します。

## C-1　独自ケース・提出前確認

`tests/student_cases.c`に自分のケースID、選定理由、初期状態、操作、期待値を追加し、`make test-student`で照合します。配布例を出発点に、自分が確かめたい条件を選んでください。公式テストの全出力の転記は不要です。図番号・ケースID・関数名を本文から参照し、不一致があれば入力・状態・コードから原因を考察します。図1・図2・表1・表2をPDFに含めます。スクリーンショットは補足として任意です。

**第6回課題の締切：Science Tokyo LMSで通知**

Science Tokyo LMSには次の2点を提出します。学籍番号は自分の番号に置き換えます。Word原稿は手元に保管してください。

| 提出名 | 内容 |
|---|---|
| `学籍番号_ex06_report.pdf` | 雛形へ記入後、PDFへ書き出し、全ページの図・表・改ページを確認 |
| `学籍番号_ex06_code.zip` | 直下に`tasks/`と`tests/`。次の3ファイルのみ |

**macOS／GNU/Linux**　リポジトリ直下で、新しい作業ディレクトリにZIPを作ります。`ID`に実際の学籍番号を設定してください。最後の3行がすべて終了0なら、展開した各ファイルの内容が一致しています。

```sh
ID=学籍番号
work=$(mktemp -d)
archive="$work/${ID}_ex06_code.zip"
zip -q "$archive" tasks/register_task.c tasks/memory_task.c tests/student_cases.c
unzip -Z1 "$archive"
mkdir "$work/readback"
unzip -q "$archive" -d "$work/readback"
cmp tasks/register_task.c "$work/readback/tasks/register_task.c"
cmp tasks/memory_task.c "$work/readback/tasks/memory_task.c"
cmp tests/student_cases.c "$work/readback/tests/student_cases.c"
```

一覧の3行は `tasks/register_task.c`、`tasks/memory_task.c`、`tests/student_cases.c` です。親フォルダー、Word、ログ、実行ファイル、`build/`、`.git/`、公式テスト・配布部品は含めません。

**Windows x64（TA実機確認待ち）**　同じ3ファイルを保持してリポジトリ直下で実行します。表示された新しいフォルダーのZIPを提出してください。

```powershell
$id = '学籍番号'
$work = New-Item -ItemType Directory -Path (Join-Path $env:TEMP ([guid]::NewGuid().ToString()))
$archive = Join-Path $work.FullName "${id}_ex06_code.zip"
$files = 'tasks/register_task.c','tasks/memory_task.c','tests/student_cases.c'
tar.exe -a -cf $archive @files
tar.exe -tf $archive
$check = New-Item -ItemType Directory -Path (Join-Path $work.FullName 'readback')
tar.exe -xf $archive -C $check.FullName
foreach ($f in $files) {
  (Get-FileHash $f).Hash -eq (Get-FileHash (Join-Path $check.FullName $f)).Hash
}
$archive
```

一覧が指定の3件で、ハッシュの照合がすべてTrueであることを確認します。質問・連絡は Slack #dp-compsys-mcs-t233-2026 へ。
