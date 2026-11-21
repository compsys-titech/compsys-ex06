# 第6回教材の保守指針

目的はC17の状態保持部品を，図・コード・状態推移・実測に対応させて学ぶこと．第5回ALU，第6回Register/Memory，第7回PC・デコード・制御の統合という範囲を保つ．命令実行，lw/sw，キャッシュ・時間遅延を追加しない．

Signal=bool，Word=Signal bit[32]，bit[0]=LSBとuint32_t変換は第5回互換．Wordのサイズを4と仮定しない．Registerは旧値を出力してから条件付き更新．RFの両ポートはread-before-writeで，x0は0・書込み無効．Decoder5・32 Register・2 MUX32の接続を保つ．if/for/配列/整数変換を使えるので第5回の演算子禁止を持ち込まない．状態をstatic/globalへ隠さない．

Memoryはuint32_t 1024語，4バイト整列．InstMemory base=0x00400000，DataMemory base=0x10010000．配布checked_memory_indexで下限→整列→上限を検査してからアクセスする．DataMemoryはchecked_data_memory_controlを先に呼ぶ．(W,R)=00はaddr未使用・無検査・出力0・状態保持，01は読出し，10は1語更新・出力0，11はCONTROLで通常復帰しない．検査前に配列へ触れない．

学生編集はtasks/register_task.c，tasks/memory_task.cのTODO内とtests/student_cases.c全体のみ．HELPERSへの関数追加，正当な別解，CRLF，空白を受理する．署名・境界外・公式テスト・配布部品は保護する．SOURCEは編集範囲の照合であり，任意の構成違反を自動証明するものではない．数値検査と構成の手動確認を分ける．テスト期待値関数を課題から呼ばない．

公開可：型/API/仕様，配布補助，未完成stub，例題ドライバー，数値テスト，runner，README．完成解答・別解・誤実装・内部採点・検証ログ・機密資料は公開しない．正解は最初からリポジトリ外の隔離コピーだけに置く．公開作業ツリーへ一時的にも置かない．レポート記入ガイド・空欄Word雛形は公開可．記入済みWord・教員TA用Excel・QA証跡はリポジトリ外に保持する．配布PPTX・PDFはDropboxの資料として管理する．

runner変更時はhelp/check/examples/all/cleanの成功，未完成の想定非0，隔離正解・別解の-O0/-O2，41機能誤実装，構成違反の手動分類，異常系，無効CC，ビルド/リンク失敗，timeout/出力上限/RESULT欠落，日本語・空白パス，読取り専用HOME，並行実行，SIGTERM/Ctrl-C，子孫とゾンビの後始末を回帰確認する．古いバイナリを再利用しない．テストで失敗した正常系を外部ラッパーで補修しない．一時領域は各run固有とし，関係ないPIDを止めない．

既存の履歴・設定・ライセンスを無断変更しない．公開範囲をdiffで確認して個別パスをstageする．正解の混入が疑われる場合は公開を止める．修正版はmacOS・Debian独立検証済み．正式な提出物・ZIP内容はREADMEとレポートガイドにそろえる．Windows x64・西8実機はTA確認待ち．実測のない環境を確認済みと記載しない．詳細配点・完成解答は本ファイルにも書かない．
