#!/bin/sh
# macOS / GNU/Linux. Make calls this from the repository root.
set -u
ulimit -c 0 2>/dev/null || :
cd "$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)" || exit 2
target=${1:-help}
case "$target" in
  help)
    cat <<'HELP'
Exercise 6 native C17 state and memory
make help/check/examples/all/test-a/test-b/test/test-student/clean
make example-register / example-register-file / example-memory
check: compile/link/RUN C17; examples: completed helpers only
all: build all programs; unfinished tasks allowed
test-a: Register/Decoder5/MUX32 -> RegisterFile; test-b: independent IM/DM
CC: a single executable path; defaults clang (macOS), cc (GNU/Linux).
CFLAGS defaults -O0 -g. C17 and warnings are always enabled.
HELP
    exit 0 ;;
  check|examples|all|test-a|test-b|test|example-register|example-register-file|example-memory|test-student|clean) ;;
  *) echo "[ENV] unknown target: $target" >&2; exit 2 ;;
esac
if [ -L build ] || { [ -e build ] && [ ! -d build ]; }; then
    echo "[ENV] build must be a real directory, not a symlink" >&2; exit 2
fi
mkdir -p build || exit 2
if [ "$target" = clean ]; then
    for item in build/run-*; do
        [ ! -L "$item" ] || { echo "[ENV] refusing symlink: $item" >&2; exit 2; }
    done
    for item in build/run-*; do
        [ ! -e "$item" ] || rm -rf -- "$item" || exit 2
    done
    echo "[PASS] clean (student files and build/.gitkeep retained)"
    exit 0
fi
run=$(mktemp -d build/run-XXXXXXXX) || exit 2
echo "LOGDIR=$run"
active='' timer=''
kill_tree()
{
    # Bootstrap only: recurse through descendants of the recorded child.
    for kt_child in $(ps -eo pid=,ppid= | awk -v p="$1" '$2 == p {print $1}'); do
        kill_tree "$kt_child"
    done
    kill -TERM "$1" 2>/dev/null || :
}
stop_timer()
{
    if [ -n "$timer" ]; then kill -TERM "$timer" 2>/dev/null || :; wait "$timer" 2>/dev/null || :; timer=''; fi
}
on_exit()
{
    trap '' HUP INT TERM
    stop_timer
    if [ -n "$active" ]; then
        kill_tree "$active"
        wait "$active" 2>/dev/null || :
    fi
}
trap 'on_exit; exit 7' HUP INT TERM
compile_seconds=${EX06_BUILD_TIMEOUT:-120}
seconds=${EX06_RUN_TIMEOUT:-20}
limit=${EX06_OUTPUT_LIMIT:-1048576}
case "$compile_seconds:$seconds:$limit" in *[!0-9:]*|:*|*::*|*:) echo "[ENV] limits must be positive integers" >&2; exit 2;; esac
if [ "$compile_seconds" -lt 1 ] || [ "$seconds" -lt 1 ] || [ "$limit" -lt 1024 ]; then
    echo "[ENV] invalid limits" >&2; exit 2
fi
if [ -z "${CC:-}" ]; then
    case "$(uname -s)" in Darwin) CC=clang;; *) CC=cc;; esac
fi
cc_path=$(command -v "$CC" 2>/dev/null) || {
    echo "[ENV] CC not found: $CC (no fallback for explicit CC)" >&2; exit 2;
}
[ -x "$cc_path" ] || { echo "[ENV] CC is not executable: $cc_path" >&2; exit 2; }
printf 'CC=%s\nCFLAGS=%s\nLDFLAGS=%s\n' "$cc_path" "${CFLAGS:--O0 -g}" "${LDFLAGS:-}" > "$run/config.txt"
"$cc_path" -std=c17 -Wall -Wextra -Wpedantic tools/guard.c -o "$run/guard" > "$run/bootstrap.stdout" 2> "$run/bootstrap.stderr" &
active=$!
printf 'pid=%s\n' "$active" > "$run/bootstrap.meta"
(
    child_sleep=''
    trap '[ -z "$child_sleep" ] || kill "$child_sleep" 2>/dev/null; wait 2>/dev/null; exit 0' TERM INT HUP
    sleep "$compile_seconds" & child_sleep=$!
    wait "$child_sleep"
    echo timeout > "$run/bootstrap.timeout"
    kill_tree "$active"
) &
timer=$!
wait "$active"; bootstrap_rc=$?; active=''
stop_timer
printf 'raw_exit=%s\n' "$bootstrap_rc" >> "$run/bootstrap.meta"
if [ "$bootstrap_rc" -ne 0 ] || [ -e "$run/bootstrap.timeout" ]; then
    cat "$run/bootstrap.stderr" >&2
    echo "[ENV] cannot build native monitor; check compiler/SDK" >&2; exit 2
fi
guard="$run/guard"
serial=0
invoke()
{
    label=$1; completion=$2; duration=$3; shift 3
    serial=$((serial + 1)); prefix="$run/$(printf '%03d' "$serial")-$label"
    "$guard" run "$duration" "$limit" "$prefix" "$completion" "$@" &
    active=$!; wait "$active"; invoked_rc=$?; active=''
    [ ! -f "$prefix.stdout" ] || cat "$prefix.stdout"
    [ ! -f "$prefix.stderr" ] || cat "$prefix.stderr" >&2
    return "$invoked_rc"
}
invoke compiler-version - "$compile_seconds" "$cc_path" --version > "$run/compiler.txt" 2>&1 || {
    echo "[ENV] compiler --version failed/timed out" >&2; exit 2;
}
invoke compiler-target - "$compile_seconds" "$cc_path" -dumpmachine > "$run/target.txt" 2>&1 || {
    echo "[ENV] compiler target query failed/timed out" >&2; exit 2;
}
# No eval: flags are whitespace-separated tokens; CC remains one exact path.
set -f
build()
{
    build_label=$1; output=$2; shift 2
    invoke "build-$build_label" - "$compile_seconds" "$cc_path" -std=c17 -Wall -Wextra -Wpedantic \
        ${CFLAGS:--O0 -g} -Iinclude -Itests "$@" ${LDFLAGS:-} -o "$output"
    result=$?
    if [ "$result" -ne 0 ]; then echo "[BUILD] $build_label compilation/link failed" >&2; return 4; fi
}
build probe "$run/probe" tools/probe.c
if [ "$?" -ne 0 ]; then echo "[ENV] native C17 probe did not build" >&2; exit 2; fi
invoke check check "$seconds" "$run/probe"
if [ "$?" -ne 0 ]; then echo "[ENV] generated C17 executable did not run correctly" >&2; exit 2; fi
[ "$target" != check ] || exit 0
"$guard" source > "$run/source.stdout" 2> "$run/source.stderr"; result=$?
cat "$run/source.stdout"; cat "$run/source.stderr" >&2
[ "$result" -eq 0 ] || exit 8
common="src/word.c src/register_support.c src/memory_support.c tasks/register_task.c tasks/memory_task.c tests/test_support.c"
build tests "$run/tests" $common tests/test_parts.c tests/test_registers.c tests/test_memory.c tests/test_errors.c tests/student_cases.c tests/test_main.c || exit 4
case "$target" in
  all|examples)
    build example-support "$run/example-support" $common tests/test_parts.c examples/support_demo.c || exit 4
    if [ "$target" = all ]; then
        build example-register "$run/example-register" $common examples/observe_register.c || exit 4
        build example-register-file "$run/example-register-file" $common examples/observe_register_file.c || exit 4
        build example-memory "$run/example-memory" $common examples/observe_memory.c || exit 4
    fi
    invoke example-support example-support "$seconds" "$run/example-support" || exit $?
    echo "[PASS] supplied helpers; observation drivers require task implementations."
    exit 0;;
  example-register|example-register-file|example-memory)
    case "$target" in
      example-register) driver=examples/observe_register.c;;
      example-register-file) driver=examples/observe_register_file.c;;
      *) driver=examples/observe_memory.c;;
    esac
    build "$target" "$run/$target" $common "$driver" || exit 4
    invoke "$target" "$target" "$seconds" "$run/$target"; exit $?;;
esac
run_group() { invoke "$1" "$1" "$seconds" "$run/tests" "$1"; }
run_errors()
{
    error_unit=$1; error_result=0
    case "$error_unit" in im|setter|index-im) base=4194304; component=InstMemory;; *) base=268500992; component=DataMemory;; esac
    k=0
    while [ "$k" -lt 6 ]; do
        case "$k" in 0) address=$((base - 4)); id=ADDR_BELOW;;
          1|2|3) address=$((base + k)); id=ADDR_ALIGN;;
          4) address=$((base + 4096)); id=ADDR_ABOVE;;
          5) address=4294967295; id=ADDR_ALIGN;; esac
        raw=10
        case "$error_unit" in control|dm-both) raw=11; id=CTRL_BOTH;; esac
        completion=$(printf 'error/%s/%s/%s/%08x/%08x' "$raw" "$id" "$component" "$address" "$base")
        invoke "E-$error_unit-$k" "$completion" "$seconds" "$run/tests" error "$error_unit" "$k" || error_result=9
        [ "$error_unit" != control ] || break
        k=$((k + 1))
    done
    return "$error_result"
}
status=0
run_group parts; parts_rc=$?
for unit in index-im index-dm setter control; do run_errors "$unit" || parts_rc=9; done
if [ "$parts_rc" -ne 0 ]; then
    echo "[DEPENDENCY] supplied helpers failed; A/B skipped, cases=0"; exit 9
fi
case "$target" in
  test-a|test|test-student)
    run_group register; register_rc=$?
    run_group decoder; decoder_rc=$?
    run_group mux; mux_rc=$?
    if [ "$register_rc:$decoder_rc:$mux_rc" = 0:0:0 ]; then run_group rf; rf_rc=$?;
    else rf_rc=9; echo "[DEPENDENCY] rf skipped: Register/Decoder5/MUX32 failed; cases=0"; fi
    if [ "$register_rc:$decoder_rc:$mux_rc:$rf_rc" != 0:0:0:0 ]; then status=9; fi
    echo "SUMMARY A register=$register_rc decoder=$decoder_rc mux=$mux_rc rf=$rf_rc";;
esac
case "$target" in
  test-b|test|test-student)
    run_group im; im_rc=$?
    if [ "$im_rc" -eq 0 ]; then run_errors im || im_rc=9;
    else echo "[DEPENDENCY] im errors skipped: normal im failed; cases=0"; fi
    run_group dm; dm_rc=$?
    if [ "$dm_rc" -eq 0 ]; then
        for unit in dm-read dm-write dm-both; do run_errors "$unit" || dm_rc=9; done
    else echo "[DEPENDENCY] dm errors skipped: normal dm failed; cases=0"; fi
    if [ "$im_rc:$dm_rc" != 0:0 ]; then status=9; fi
    echo "SUMMARY B im=$im_rc dm=$dm_rc (independent of A)";;
esac
if [ "$target" = test-student ]; then
    if [ "$rf_rc" -eq 0 ]; then run_group student-a || status=9;
    else echo "[DEPENDENCY] student-a skipped; cases=0"; fi
    if [ "$im_rc:$dm_rc" = 0:0 ]; then run_group student-b || status=9;
    else echo "[DEPENDENCY] student-b skipped; cases=0"; fi
fi
if [ "$status" -eq 0 ]; then echo "[PASS] $target completed"; fi
exit "$status"
