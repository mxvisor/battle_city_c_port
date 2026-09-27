#!/usr/bin/env bash
# Регрессионный тест по кадрам: прогоняет сценарии в детерминированном
# headless-режиме (--test-frames) и сравнивает хэши кадров/записей APU
# с эталоном в golden/.
#
#   tests/regression/run.sh build/battle_city            # проверка
#   tests/regression/run.sh build/battle_city --update   # перезаписать эталон
#
# DUMP_DIR=dir — при расхождении сохранить BMP-кадры упавших сценариев в dir/<сценарий>/.
set -u

BIN=${1:?usage: run.sh <battle_city binary> [--update]}
MODE=${2:-check}
HERE=$(cd "$(dirname "$0")" && pwd)
EVERY=30

# имя            кадров  файл ввода (- = без ввода)
SCENARIOS=(
  "attract       3600    -"
  "play_1p       4800    play_1p.input"
  "construction  1800    construction.input"
)

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fail=0

for s in "${SCENARIOS[@]}"; do
  read -r name frames input <<<"$s"
  args=(--test-frames "$frames" --test-every "$EVERY")
  [ "$input" != "-" ] && args+=(--test-input "$HERE/$input")
  golden="$HERE/golden/$name.txt"

  if ! "$BIN" "${args[@]}" --test-out "$tmp/$name.txt" >/dev/null; then
    echo "FAIL $name: binary exited with error"; fail=1; continue
  fi

  if [ "$MODE" = "--update" ]; then
    mkdir -p "$HERE/golden"
    cp "$tmp/$name.txt" "$golden"
    echo "UPDATED $name"
    continue
  fi

  if cmp -s "$tmp/$name.txt" "$golden"; then
    echo "OK   $name"
    continue
  fi

  fail=1
  first=$(diff <(cut -d' ' -f1,2 "$golden") <(cut -d' ' -f1,2 "$tmp/$name.txt") | grep -m1 '^[<>]' | awk '{print $2}')
  first_apu=$(diff <(cut -d' ' -f1,3 "$golden") <(cut -d' ' -f1,3 "$tmp/$name.txt") | grep -m1 '^[<>]' | awk '{print $2}')
  echo "FAIL $name: first frame mismatch at ${first:-none}, first APU mismatch at ${first_apu:-none}"
  diff "$golden" "$tmp/$name.txt" | head -8
  if [ -n "${DUMP_DIR:-}" ]; then
    mkdir -p "$DUMP_DIR/$name"
    "$BIN" "${args[@]}" --test-out /dev/null --test-dump "$DUMP_DIR/$name" >/dev/null
    echo "     frames dumped to $DUMP_DIR/$name"
  fi
done

exit $fail
