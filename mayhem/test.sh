#!/usr/bin/env bash
#
# mayhem/test.sh — RUNS a set of ViSP's own Catch2 unit tests (built by mayhem/build.sh with normal
# flags into build/test; list in mayhem/tests.txt). Each binary writes a JUnit report; counts come from
# the reported test cases and failures, so a binary that produces no report (e.g. a no-op program)
# counts as a failure. Exit 0 iff no test failed.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

passed=0; failed=0
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
for t in $(cat "$SRC/mayhem/tests.txt"); do
  bin=$(find "$SRC/build/test" -type f -name "$t" -perm -u+x 2>/dev/null | head -n1)
  if [ -z "$bin" ]; then
    echo "MISSING test binary $t (build.sh should have built it)" >&2
    failed=$((failed + 1)); continue
  fi
  rep="$work/$t.xml"
  (cd "$work" && "$bin" --reporter "junit::out=$rep" >"$work/$t.log" 2>&1)
  rc=$?
  cases=$(grep -o '<testcase ' "$rep" 2>/dev/null | wc -l)
  fails=$(grep -c -E '<(failure|error)( |>)' "$rep" 2>/dev/null || true)
  : "${fails:=0}"
  if [ "$cases" -eq 0 ]; then
    echo "FAIL $t: no test cases reported (rc=$rc)" >&2; tail -n 20 "$work/$t.log" >&2
    failed=$((failed + 1)); continue
  fi
  if [ "$rc" -ne 0 ] && [ "$fails" -eq 0 ]; then fails=1; fi
  echo "$t: $cases cases, $fails failed (rc=$rc)"
  [ "$fails" -gt 0 ] && tail -n 40 "$work/$t.log" >&2
  passed=$((passed + cases - fails)); failed=$((failed + fails))
done
emit_ctrf "catch2" "$passed" "$failed" 0
