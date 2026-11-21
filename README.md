# 計算機システム（演習）2026年度 第6回

目次

- [1．目的と第5～7回の位置付け](#purpose)
- [2．必要ツール](#tools)
- [3．確認済み環境](#environment)
- [4．取得・更新](#checkout)
- [5．ファイル構成と編集範囲](#files)
- [6．配布部品・課題A・課題B](#parts)
- [7．型と公開API](#types)
- [8．read-before-write](#timing)
- [9．x0](#zero)
- [10．5ビット番号の配列順](#select)
- [11．アドレスと添字](#address)
- [12．DataMemoryの制御表](#control)
- [13．Makeターゲット](#commands)
- [14．配布版の正常終了と未完成](#starter)
- [15．例題1～3](#examples)
- [16．公式テストと学生独自テスト](#student)
- [17．エラー分類とログ](#errors)
- [18．レポートで記録するもの](#report)
- [19．トラブルシューティング](#trouble)
- [20．提出物と締切](#submission)
- [21．出典](#sources)

<a id="purpose"></a>
## 1．目的と第5～7回の位置付け

第5回で作ったALUは組み合わせ回路です．第6回では，値を次の呼出しまで保持するRegister・RegisterFile・InstMemory・DataMemoryを，Cの構造体と関数で表します．関数呼出し前の状態，読出し出力，呼出し後の状態を分けて記録します．電圧，クロック内の伝播遅延，キャッシュ，実機の待ち時間はモデル化しません．

第7回でPC・デコード・制御と接続します．今回は命令語を読み出すところまでで，命令実行ループや`lw`／`sw`は作りません．メモリ配置と呼出し時のタイミングは本演習の契約です．ISAが物理レジスタファイルのread-before-writeや命令・データの別配列を要求するという意味ではありません．このリポジトリだけでビルドできます．

<a id="tools"></a>
## 2．必要ツール

Git，GNU Make 3.81以降，C17対応の**ホスト用Cコンパイラ**，OS標準シェルが必要です．macOSはApple Clang，GNU/LinuxはGCCまたはClang，Windows x64はPowerShell 5.1以降とMinGW-w64系GCCを想定しています．

RISC-Vクロスコンパイラ，RARS，Java，QEMU，GDB，Python，Node.js，Docker，CMake，外部テストライブラリは不要です．32ビット語を扱うことと，ホスト実行ファイルを32ビット化することは別です．

macOSで既存ツールを確認します．

```sh
git --version
make --version
xcode-select -p
clang --version
make check
```

通常はPATH上の`clang`または`/usr/bin/clang`を使います．AppleのドライバーがSDKを選択します．不足する場合は教員・TAに相談してください．`make check`は版表示だけでなくC17プログラムをコンパイル・リンクし，そのPCで実行します．

GNU/Linuxでは既存の`cc`を使います．`make CC=gcc check`，Clang導入済みなら`make CC=clang check`で明示できます．

### Windows x64の固定候補（TA確認待ち）

第5回の固定候補を継承します．既存の第1回用Makeは再利用できますが，RISC-V GCCはこのC教材を動かすコンパイラにはなりません．WSL，Git Bash，MSYS2シェルは必須ではありません．

- [WinLibs公式](https://winlibs.com/)のWin64／UCRT／POSIX版
- tag: `16.2.0posix-23.1.3-14.0.0-ucrt-r3`
- [公式リリース](https://github.com/brechtsanders/winlibs_mingw/releases/tag/16.2.0posix-23.1.3-14.0.0-ucrt-r3)
- [ZIP](https://github.com/brechtsanders/winlibs_mingw/releases/download/16.2.0posix-23.1.3-14.0.0-ucrt-r3/winlibs-x86_64-posix-seh-gcc-16.2.0-mingw-w64ucrt-14.0.0-r3.zip)
- [配布元SHA-512](https://github.com/brechtsanders/winlibs_mingw/releases/download/16.2.0posix-23.1.3-14.0.0-ucrt-r3/winlibs-x86_64-posix-seh-gcc-16.2.0-mingw-w64ucrt-14.0.0-r3.zip.sha512)

配布元SHA-512の固定値：

```text
fdd7a649946bd0aaa50585b9902b91f58c9f7caa6de29901e46013b9f9f4187b8bc16dfb6beecadd12141f6417a044a54c63956bf34cdac97bcd5f724d100997
```

取得ファイルの値を照合してから，教材外のユーザー領域へ展開します．

```powershell
Get-FileHash -Algorithm SHA512 -LiteralPath "$HOME\Downloads\winlibs-x86_64-posix-seh-gcc-16.2.0-mingw-w64ucrt-14.0.0-r3.zip"
$compiler = Join-Path $HOME 'tools\winlibs-ex06\mingw64\bin\gcc.exe'
$makeExe = Join-Path $HOME 'tools\winlibs-ex06\mingw64\bin\mingw32-make.exe'
Test-Path -LiteralPath $compiler
Test-Path -LiteralPath $makeExe
& $compiler --version
& $makeExe --version
$env:CC = $compiler
& $makeExe check
& $makeExe examples
& $makeExe all
```

実際の展開先に合わせて変更し，両方の`Test-Path`が`True`であることを確認します．既存の`make.exe`も実在するフルパスで指定できます．以下の`make`は`& $makeExe`へ読み替えます．恒久PATH変更は不要です．runnerはDLL探索のため，当該PowerShellプロセス内だけでコンパイラのディレクトリをPATHに加えます．

<a id="environment"></a>
## 3．確認済み環境

| 環境 | 状態 |
|---|---|
| macOS 27.2.1（26B131），Apple M4／arm64，Mac16,12 | ネイティブC17を検証 |
| Apple Clang 17.0.0（clang-1700.6.3.2），target arm64-apple-darwin27.2.0，GNU Make 3.81 | `-O0`／`-O2`で検証 |
| Debian GNU/Linux 13.6／x86_64，Intel Xeon Platinum 8573C | GCC 14.2.0，GNU Make 4.4.1で独立検証．Clangは未導入・未確認 |
| Windows x64 | TA確認待ち・未確認 |
| 西8実習室GNU/Linux | TA確認待ち・未確認 |

macOSとDebianの各環境で正解・別解の通常回帰84/84，CRLF別解の`-O2 make test`連続30/30（各2,535比較），機能誤実装41件の検出を確認しました．最終RESULTの出力回収，timeout・SIGTERM・Ctrl-C後の子孫回収も確認しました．指定部品の構成は別途ソースで確認します．

DebianではGCCのASan＋UBSanを確認しました．macOSでは異常系・実行条件31件，既定Clang 17のUBSan単独，Xcode Clang 21／SDK 27.2のASan＋UBSanを確認しました．既定Clang 17のASan＋UBSanはC17プローブが時間上限に達しました．

<a id="checkout"></a>
## 4．取得・更新

```sh
git clone https://github.com/compsys-titech/compsys-ex06.git
cd compsys-ex06
make
make check
make examples
make all
```

無引数`make`はhelpです．更新前は`git status --short`と`git diff`で自分の変更を確認し，編集ファイルを教材外へバックアップします．変更がなければ`git pull --ff-only`で更新できます．変更がある場合や更新が拒否された場合は，コードを保持したままTAに相談してください．

<a id="files"></a>
## 5．ファイル構成と編集範囲

| ファイル | 役割 |
|---|---|
| [include/compsys.h](include/compsys.h) | 型と公開API．変更不要 |
| [src/word.c](src/word.c)，[src/register_support.c](src/register_support.c)，[src/memory_support.c](src/memory_support.c) | 完成済み配布部品 |
| [tasks/register_task.c](tasks/register_task.c) | 課題A．TODO内だけ編集 |
| [tasks/memory_task.c](tasks/memory_task.c) | 課題B．TODO内だけ編集 |
| [tests/student_cases.c](tests/student_cases.c) | 独自テスト．**このファイル全体を編集可** |
| `examples/support_demo.c`，`examples/observe_*.c` | 配布部品例と実装観察用ドライバー |
| `tests/test_parts.c`，`test_registers.c`，`test_memory.c`，`test_errors.c` | 公式数値検査と例外検査 |
| `tests/test_support.*`，`test_main.c` | 比較・表示・検査入口 |
| `tests/*_task.original.c`，`tests/protected.fnv` | 編集境界の原本と配布ファイル照合 |
| `Makefile`，`tools/run.sh`，`tools/run.ps1` | OS別入口 |
| `tools/guard.c`，`tools/probe.c` | 実行監視・SOURCE検査・C17確認 |
| `build/run-*` | 毎回生成する実行ファイルとログ．Git管理対象外 |

編集するのは課題2ファイルのTODO内と`tests/student_cases.c`です．境界名はA：`A_HELPERS / REGISTER / DECODER5 / MUX32 / REGISTER_FILE`，B：`B_HELPERS / INST_MEMORY / DATA_MEMORY`です．`/* TODO-BEGIN REGISTER */`と`/* TODO-END REGISTER */`などを残します．補助関数はHELPERS内へ置けます．関数宣言，境界外，配布部品，公式テストは変更しません．LF／CRLFの違いは受理します．

<a id="parts"></a>
## 6．配布部品・課題A・課題B

配布済み：Word変換，Register初期値のsetter/getter，命令語初期化，アドレス検査，不正なDataMemory制御の検査．これらは課題の読出し・状態更新を代行しません．

課題A：`register_run`，`decoder5`，`mux32`，`register_file_run`．RegisterFileは**Decoder5・32個のRegister・2個のMUX32の接続**で作ります．各Registerの更新前出力をWord配列として保存し，両読出しポートへ渡す構成を図とコードで示します．直接添字だけでRF全体を代用すると，数値が合っても構成条件を満たしません．

課題B：`inst_memory_run`と`data_memory_run`．課題Aに依存せず検証できます．有効アクセス時は配布アドレス検査を使い，DataMemoryは制御検査を先に使います．状態は渡された構造体に保存し，別インスタンスと共有するstatic/global状態へ隠しません．

今回は`if`／`for`，配列，添字計算，Wordコピー・整数変換を使用できます．第5回のゲート接続課題の演算子禁止を流用しません．テスト期待値関数の呼出しや入力別の答え一覧で実装を代用しません．数値検査と構成の手動確認は別です．

<a id="types"></a>
## 7．型と公開API

```c
typedef bool Signal;
typedef struct { Signal bit[32]; } Word;
typedef struct { uint32_t val; } Register;
typedef struct { Register r[32]; } RegisterFile;
#define COMPSYS_MEMORY_WORDS 1024u
typedef struct { uint32_t mem[COMPSYS_MEMORY_WORDS]; } InstMemory;
typedef struct { uint32_t mem[COMPSYS_MEMORY_WORDS]; } DataMemory;
```

`Signal`と`Word`は第5回と互換です．`bit[0]`がLSBです．`Word`は32本の論理配線であり，`sizeof(Word)==4`ではありません．メモリの1024個の`uint32_t`は1024語＝4096バイトをモデル化します．

公開関数の正確な署名は[compsys.h](include/compsys.h)を参照します．

| 関数 | 契約 |
|---|---|
| `word_set_value`／`word_get_value` | `uint32_t`と32本のSignalを変換 |
| `register_set_value`／`register_get_value` | 初期化・観察用．一般Registerは0以外も保存 |
| `register_run` | 旧値を出力し，wctlがtrueならwdataへ更新 |
| `decoder5` | 5本からone-hotのWordを作る |
| `mux32` | 32個の32ビットWordから1個を選ぶ |
| `register_file_run` | 2読出し・1書込み．両読出しとも更新前 |
| `inst_memory_set_inst` | バイトアドレスを検査し，指定1語を初期化 |
| `inst_memory_run` | 指定語を出力し，命令メモリを変更しない |
| `data_memory_run` | `(mem_write, mem_read)`に従い読出し・更新 |
| `checked_memory_index` | 下限，整列，上限を検査し語添字を返す |
| `checked_data_memory_control` | 両制御trueをCONTROLとして異常終了 |

使用前に`Register reg={0}; RegisterFile rf={0}; InstMemory im={0}; DataMemory dm={0};`などで初期状態を明示します．出力は毎回全32ビットを定義します．ポインタは有効で，入力配列・各出力先は重ならないものとします．NULLやaliasへの対応は課題外です．

<a id="timing"></a>
## 8．read-before-write

`reg->val=10`，`wctl=1`，`wdata=20`なら，その呼出しの`rdata=10`，呼出し後の`reg->val=20`です．次の呼出しで20を読めます．`wctl=0`ではwdataを無視して旧値を保持します．

RegisterFileも同じです．`read1`や`read2`が`write1`と一致しても，**両ポートとも呼出し前の値**を出力します．呼出しをCPUの1クロックと定義するものではありません．

<a id="zero"></a>
## 9．x0

RegisterFileのx0は常に0を読み，x0への書込みを無視します．両読出しポートで守ります．これはRFの条件です．一般Register自体に「値0を保存できない」「必ず0」といった規則を加えません．

<a id="select"></a>
## 10．5ビット番号の配列順

`read1[0]`，`read2[0]`，`write1[0]`，Decoderの`a[0]`，MUXの`ctls[0]`がいずれもLSBです．番号3は配列`{1,1,0,0,0}`，番号31は`{1,1,1,1,1}`です．Decoder5は指定bitだけtrue，他の31本はfalseにします．MUX32は32入力それぞれが32ビットです．前回の出力を残さず，全出力を更新します．

<a id="address"></a>
## 11．アドレスと添字

| メモリ | base | 最終語のバイトアドレス |
|---|---|---|
| InstMemory | `INST_MEM_START = 0x00400000` | `0x00400ffc` |
| DataMemory | `DATA_MEM_START = 0x10010000` | `0x10010ffc` |

有効な語先頭アドレスは`base`から`base+4092`まで4刻みです．`base / base+4 / base+8 / base+4092`は添字`0 / 1 / 2 / 1023`に対応します．入力はバイトアドレスで，添字そのものではありません．配置値は教材の設定です．

`checked_memory_index(address, base, component)`は，まず`address < base`を拒否し，次に差分の4バイト整列，最後に語添字が1024未満かを確認します．不正な入力ではstderrにADDRESS・安定ID・component・入力address・baseを出し，終了値10で通常復帰しません．検査より前に配列へアクセスしません．`UINT32_MAX`は整列違反です．

`inst_memory_set_inst`は検査した1語だけを書き換えます．命令例`0x007302b3`を読み出せればよく，命令デコードや実行を課題Bへ加えません．

<a id="control"></a>
## 12．DataMemoryの制御表

引数順は**mem_write，mem_read**です．

| mem_write | mem_read | 出力rdata | 状態・検査 |
|---:|---:|---|---|
| 0 | 0 | 必ず0 | 無操作．addrを使わず検査もしない．全状態保持 |
| 0 | 1 | 指定語 | アドレス検査後に読出し．全状態保持 |
| 1 | 0 | 必ず0 | アドレス検査後，指定1語のみwdataへ更新 |
| 1 | 1 | 通常復帰しない | CONTROL／CTRL_BOTH，終了値11．メモリへアクセスしない |

両制御trueはアドレス検査より先に拒否します．不正アドレスを同時に渡してもCONTROLです．無操作で不正アドレスを渡しても成功します．これらは本演習モデルの仕様であり，RISC-Vのメモリ例外仕様全体ではありません．

<a id="commands"></a>
## 13．Makeターゲット

| コマンド | 内容 |
|---|---|
| `make`，`make help` | help．コンパイラ不要 |
| `make check` | C17のコンパイル・リンク・ネイティブ実行 |
| `make examples` | 完成済み配布補助だけを実行 |
| `make all` | 全プログラムをビルドし，配布補助を実行 |
| `make test-a` | Register／Decoder5／MUX32，成功後にRegisterFile |
| `make test-b` | InstMemoryとDataMemory．Aは実行しない |
| `make test` | Aが失敗してもBまで報告 |
| `make example-register` | 自分のRegisterを観察 |
| `make example-register-file` | 自分のRegisterFileを観察 |
| `make example-memory` | 自分のInstMemory・DataMemoryを観察 |
| `make test-student` | 公式A/Bと，対応する実装が合格した独自テスト |
| `make clean` | 生成したrunディレクトリのみ削除．`.gitkeep`と学生ファイルを保持 |

`make CC=/usr/bin/clang CFLAGS='-O2 -g' test`のように指定できます．CCは単一の実行ファイル名／パスです．空白を含むパスは`make CC='/path with space/clang' check`のように一つの引数にします．コマンド列や`ccache clang`をCCに入れません．CFLAGS/LDFLAGSは空白区切りのフラグです．`eval`しません．C17と警告フラグは常に加えます．

<a id="starter"></a>
## 14．配布版の正常終了と未完成

| ターゲット | 配布直後 runner終了値 | Make終了値 |
|---|---:|---:|
| help，check，examples，all，clean | 0 | 0 |
| test-a，test-b，test，test-student | 9（集約不合格） | 2 |
| example-register，example-register-file，example-memory | 3（UNFINISHED） | 2 |

課題はコンパイルできる未完成stubで，呼ばれるとUNFINISHEDを表示して終了します．`all`成功は課題完成を意味しません．未完成表示を削除しただけでも機能検査には合格しません．依存部品の失敗で省略したグループは`DEPENDENCY ... cases=0`となり，合格件数へ数えません．

完成時は各テスト・観察ターゲットが0です．Makeの2と，子プログラムやrunnerの非0値を区別してください．直接`sh tools/run.sh test-a`（Windowsは`powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/run.ps1 test-a`）でrunner値を確認できます．

<a id="examples"></a>
## 15．例題1～3

最初に`make examples`で配布補助を確認します．以下の観察ドライバーは自分の課題関数を直接呼びます．期待値から実出力を作る代替経路はありません．

例題1（p.14）：Registerを0から始め，`(wctl,wdata)=(1,100),(0,999),(1,200),(0,0)`とします．期待出力は`0,100,100,200`，呼出し後状態は`100,100,200,200`です．`make example-register`で実出力を比較します．

例題2（p.24）：RegisterFileを0で初期化し，`read1=read2=write1=1`で同じ制御列を与えます．両ポートの期待出力は同じ列です．`make example-register-file`を実行します．別番号への2読出し・x0・書込み無効も独自に追加します．

例題3（p.30，33）：InstMemoryの先頭から`0x007302b3`を読みます．DataMemoryの`base+4`へ100，`base+8`へ200を書き，逆順に200，100を読みます．書込み時の出力は0です．`make example-memory`で記録します．この段落の値は仕様からの期待値です．自分の実行ログを実測として示してください．

<a id="student"></a>
## 16．公式テストと学生独自テスト

公式検査は各出力の全32ビットと状態を比較します．Registerの独立インスタンス，Decoder全32番号と再利用，MUX全32入力，RFの両ポート・同一番号の読書き・選択外の状態，メモリの先頭／末尾／単一bit／保持を含みます．IMは固定seed `0x20260601`，漸化式`x = x * 1664525 + 1013904223`（uint32_t）の1024語も確認します．OSのrand列に依存しません．

異常系は別プロセスで終了値10／11と診断ID・component・アドレスを照合します．C17の`atexit`で終了時点の全メモリ不変も比較します．これは一時的な読出しが一切なかったことの証明ではありません．検査前にメモリへ触れない構成はソースで確認します．

`tests/student_cases.c`の`student_a`と`student_b`を編集し，識別名，初期状態，呼出し順，入力，制御，期待値，実値，呼出し後状態，合否，選定理由を追加します．`test_case`／`test_state`は比較・表示用です．実装本体からテスト側関数を呼びません．配布された2例だけでレポート完成にはなりません．公式テストを書き換えず追加できます．

ケース数は比較の単位で，入力の網羅性や点数ではありません．同じ識別名を図・記録表・本文から参照してください．

<a id="errors"></a>
## 17．エラー分類とログ

| 分類 | runner／生終了値 | 意味 |
|---|---:|---|
| PASS | 0 | 最終RESULTまで確認 |
| ENV | 2 | CC・SDK・C17・ホスト実行・設定の問題 |
| UNFINISHED | 3 | 課題stubが実行された |
| BUILD | 4 | コンパイル／リンク失敗 |
| FAIL | 5 | 出力・状態・期待例外が不一致 |
| TIMEOUT | 6 | 実行時間上限 |
| RUNTIME | 7 | 異常終了，出力超過，最終RESULT欠落，割込み等 |
| SOURCE | 8 | 境界・署名・配布ファイル変更 |
| 集約不合格 | 9 | 課題ターゲットに失敗／依存省略あり |
| ADDRESS | 生10 | ADDR_BELOW／ADDR_ALIGN／ADDR_ABOVE |
| CONTROL | 生11 | CTRL_BOTH |

期待した例外を公式の隔離ケースで確認できた場合，そのケースはPASSです．それ以外の生10／11は通常実行のRUNTIMEとして記録されます．終了値だけを0へ置換しません．

各実行は新しい`build/run-*`を作り，`config.txt`，コンパイラ・target表示，SOURCE結果，各プロセスのstdout／stderr／metaを保存します．metaに実コマンド，PID（POSIXではPGID），時間上限，出力上限，生終了値，runner値，経過時間，wait完了を記録します．古いバイナリを再利用しません．

通常実行20秒，ビルド120秒，監視対象1プロセスのstdout＋stderrは1 MiBです．必要時は`EX06_RUN_TIMEOUT`，`EX06_BUILD_TIMEOUT`，`EX06_OUTPUT_LIMIT`を明示できます．timeoutや割込みでは自分が起動したプロセスグループ／Windows Jobを停止・回収します．無関係なプロセス名による一括killはしません．この監視は悪意あるCコードに対するセキュリティsandboxではありません．

<a id="report"></a>
## 18．レポートで記録するもの

[正式レポート記入ガイド](report/README.md)と[空欄Word雛形](report/ex06-report-template.docx)を使い，次を図・コード・実行結果で対応付けます．

- RegisterFile内部接続図：Decoder5，32個のRegister，2個のMUX32，x0，配線変数と制御値．
- Register／RegisterFile状態推移記録：呼出し順，入力，制御，前状態，期待出力，実出力，後状態，合否，選定理由．
- バイトアドレス対応図：base，4バイト刻み，添字0／1／2／1023，容量と不正範囲．
- メモリ読書き記録：制御，入力アドレス，添字，前の値，期待値・実値，後状態，非選択語の保持，エラーの確認．
- 図番号・ケースIDを本文から参照し，関数・変数・配列・条件と対応させた説明．

状態の前後，指定部品の接続，x0，アドレス変換，制御条件，独自テストの理由を確認します．スクリーンショットは任意で，図や記録表の代わりではありません．数値テストの合格だけで構成や説明の適合，成績が確定するわけではありません．

<a id="trouble"></a>
## 19．トラブルシューティング

`make check`がENVなら，まずCCがホスト用か確認します．明示したCCが不正な場合，別コンパイラへ自動fallbackしません．`make help`／`make clean`はCC不要です．

BUILDは最初のコンパイル／リンク診断，SOURCEは変更されたファイル・境界名を読みます．自分のコードをバックアップしてTAへ相談し，公式テストや保護情報を変更して回避しません．UNFINISHEDは環境不良ではありません．

FAILは最初の安定IDと入力・expected・actual・before・afterを確認します．RFがDEPENDENCYならRegister／Decoder／MUXから直します．BはAから独立です．無操作DataMemoryのADDRESSは仕様違反の兆候です．

TIMEOUTや出力超過はループ条件・出力回数を確認します．最終RESULTがない終了0も成功扱いしません．`build`がsymlink／reparse pointなら処理を拒否します．教材本体を実ディレクトリへ取得して使います．`make clean`は学生ソースを消しません．

<a id="submission"></a>
## 20．提出物と締切

Science Tokyo LMSに`学籍番号_ex06_code.zip`と`学籍番号_ex06_report.pdf`を提出します．**第6回課題の締切：Science Tokyo LMSで通知**

ZIPには直下から`tasks/register_task.c`，`tasks/memory_task.c`，`tests/student_cases.c`の3件だけを格納します．親フォルダーを付けず，図1・図2と表1・表2は提出PDFに含めます．Word原稿は手元に保管します．作成・展開確認の手順は[レポート記入ガイド](report/README.md)を参照してください．

<a id="sources"></a>
## 21．出典

- 2026年度「計算機システム（演習）」第6回 `Compsys2026_ex06.pptx`，全38枚の本文・発表者ノート．今回のコードと検査はこの部品仕様に対応します．
- [第5回の固定参照commit](https://github.com/compsys-titech/compsys-ex05/tree/0402dc01dca674682d64f18f0ddc06df2430c01a)：Signal／Word，ネイティブC17とrunner，固定WinLibs候補を継承．ALU課題や演算子制約は継承しません．
- [RISC-V公式 RV32I](https://docs.riscv.org/reference/isa/v20260120/unpriv/rv32.html)：32本の整数レジスタ，32ビット幅，x0の仕様．本教材のタイミング・メモリ配置・制御表は演習モデルの契約です．
