#!/usr/bin/env bash
set -euo pipefail
arena_root="$(cd -- "$(dirname -- "$0")/../.." && pwd)"
arena_local="$arena_root/.arena-dev"
mkdir -p "$arena_local"
cd "$arena_root"
export LC_ALL=C
# Matt mod: MATT_PART=2 builds the Part 2 preview (own roster + wild table, same save layout).
matt_part="${MATT_PART:-1}"
if [[ "$matt_part" == 3 ]]; then
  roster=tools/arena/roster_part3.json; wild_json=src/data/wild_encounters_part2.json; rel_name=pokeemerald-part3; rel_dir=build-part3
elif [[ "$matt_part" == 2 ]]; then
  roster=tools/arena/roster_part2.json; wild_json=src/data/wild_encounters_part2.json; rel_name=pokeemerald-part2; rel_dir=build-part2
else
  roster=tools/arena/roster.json; wild_json=src/data/wild_encounters.json; rel_name=pokeemerald; rel_dir=build
fi
build_background() {
  mkdir -p .arena-dev/art
  if [[ ! -f .arena-dev/art/space.4bpp || ! -f .arena-dev/art/space.bin || ! -f .arena-dev/art/space.gbapal || ! -f .arena-dev/art/crater.4bpp || tools/arena/pack_toss_pr1.c -nt .arena-dev/art/space.4bpp ]]; then
    cc -std=c11 -O2 -Wall -Wextra  tools/arena/pack_toss_pr1.c  -lpng -lm -o .arena-dev/art/pack-toss-pr1
    .arena-dev/art/pack-toss-pr1 .arena-dev/art/pr1-actions.4bpp .arena-dev/art/pr1-bolts.4bpp .arena-dev/art/crater.4bpp .arena-dev/art/space.4bpp .arena-dev/art/space.bin .arena-dev/art/space.gbapal
  fi
  if [[ ! -f .arena-dev/art/props.4bpp || ! -f .arena-dev/art/prop-palettes.gbapal || tools/arena/pack_props.c -nt .arena-dev/art/props.4bpp || graphics/arena/forest-props-sunburst-v1.png -nt .arena-dev/art/props.4bpp ]]; then
    cc -std=c11 -O2 -Wall -Wextra  tools/arena/pack_props.c  -lpng -lm -o .arena-dev/art/pack-props
    .arena-dev/art/pack-props graphics/arena/forest-props-sunburst-v1.png .arena-dev/art/props.4bpp .arena-dev/art/pieces.4bpp .arena-dev/art/blast.4bpp .arena-dev/art/prop-palettes.gbapal
  fi
  if [[ ! -f .arena-dev/art/actions.4bpp || tools/arena/pack_move_fx.c -nt .arena-dev/art/actions.4bpp || graphics/arena/cc0/trace_01.png -nt .arena-dev/art/actions.4bpp ]]; then
    cc -std=c11 -O2 -Wall -Wextra  tools/arena/pack_move_fx.c  -lpng -lm -o .arena-dev/art/pack-move-fx
    .arena-dev/art/pack-move-fx .arena-dev/art/actions.4bpp .arena-dev/art/bolts.4bpp
  fi
  if [[ ! -f .arena-dev/art/forest.8bpp || graphics/arena/forest-clearing-v2-clean.png -nt .arena-dev/art/forest.8bpp || tools/arena/pack_background.c -nt .arena-dev/art/forest.8bpp ]]; then
    cc -std=c11 -O2 -Wall -Wextra  tools/arena/pack_background.c  -lpng -lm -o .arena-dev/art/pack-background
    .arena-dev/art/pack-background graphics/arena/forest-clearing-v2-clean.png .arena-dev/art/forest.8bpp .arena-dev/art/forest.gbapal .arena-dev/art/forest.bin
  fi
  for biome in coast cave desert gym; do
    prop_source="graphics/arena/props-$biome-v2.png"
    stage_source="graphics/arena/biome-$biome-v1.png"
    if [[ "$biome" == gym && -f graphics/arena/props-gym-pmd-v3.png && -f graphics/arena/biome-gym-pmd-v3.png ]]; then
      prop_source=graphics/arena/props-gym-pmd-v3.png
      stage_source=graphics/arena/biome-gym-pmd-v3.png
    fi
    if [[ ! -f .arena-dev/art/$biome-props.4bpp || "$prop_source" -nt .arena-dev/art/$biome-props.4bpp || tools/arena/pack_props.c -nt .arena-dev/art/$biome-props.4bpp ]]; then
      cc -std=c11 -O2 -Wall -Wextra  tools/arena/pack_props.c  -lpng -lm -o .arena-dev/art/pack-props
      .arena-dev/art/pack-props "$prop_source" .arena-dev/art/$biome-props.4bpp .arena-dev/art/$biome-pieces.4bpp .arena-dev/art/$biome-blast.4bpp .arena-dev/art/$biome-prop-palettes.gbapal
    fi
    if [[ ! -f .arena-dev/art/$biome.8bpp || "$stage_source" -nt .arena-dev/art/$biome.8bpp || tools/arena/pack_background.c -nt .arena-dev/art/$biome.8bpp ]]; then
      cc -std=c11 -O2 -Wall -Wextra  tools/arena/pack_background.c  -lpng -lm -o .arena-dev/art/pack-background
      .arena-dev/art/pack-background "$stage_source" .arena-dev/art/$biome.8bpp .arena-dev/art/$biome.gbapal .arena-dev/art/$biome.bin
    fi
  done
  # Community stages: originals remain outside Git; adapted PNGs are inputs.
  community_repack=0
  for source in tools/arena/import_oswarlin.py graphics/arena/oswarlin/*.png; do
    if [[ ! -f graphics/arena/oswarlin/flood.8bpp || "$source" -nt graphics/arena/oswarlin/flood.8bpp ]]; then community_repack=1; fi
  done
  if [[ "$community_repack" == 1 ]]; then
    community_python="${ARENA_PYTHON:-python3}"
    if ! "$community_python" -c 'import PIL' >/dev/null 2>&1; then
      if [[ -x ../venv/bin/python ]]; then community_python=../venv/bin/python;
      else printf '%s\n' 'Community art needs Pillow: set ARENA_PYTHON to its Python environment.'; exit 1; fi
    fi
    "$community_python" tools/arena/import_oswarlin.py
    touch src/arena_stage_art.c src/realtime_arena.c
  fi
  # INCBIN resources inside .inc files are not discovered by the upstream
  # dependency scanner. Keep the derived flood floor/contact assets current.
  storm_repack=0
  for storm_input in tools/arena/pack_storm.py graphics/arena/storm/*.png .arena-dev/art/*.8bpp .arena-dev/art/*.gbapal .arena-dev/art/*props.4bpp; do
    if [[ ! -f graphics/arena/storm/backgrounds.8bpp || "$storm_input" -nt graphics/arena/storm/backgrounds.8bpp ]]; then storm_repack=1; fi
  done
  if [[ "$storm_repack" == 1 ]]; then
    storm_python="${ARENA_PYTHON:-python3}"
    if ! "$storm_python" -c 'import PIL' >/dev/null 2>&1; then
      if [[ -x ../venv/bin/python ]]; then storm_python=../venv/bin/python;
      else printf '%s\n' 'Storm art needs Pillow: set ARENA_PYTHON to its Python environment.'; exit 1; fi
    fi
    "$storm_python" tools/arena/pack_storm.py
  fi
}
case "${1:-help}" in
  assets)
    python3 tools/arena/import_sprites.py --catalog "$roster"
    python3 tools/arena/pack_species_dict.py
    printf '%s\n' "$matt_part" > .arena-dev/pmd/matt_part
    build_background
    ;;
  build)
    mkdir -p "$arena_root/.arena-dev"
    test -f .arena-dev/pmd/sprites.inc || { printf '%s\n' 'Import sprite assets first: ./tools/arena/dev.sh assets'; exit 1; }
    build_background
    make -j4 CPP=cpp ARENA_LAB=1 FILE_NAME=arena_lab BUILD_DIR=build-lab > "$arena_root/.arena-dev/build.log" 2>&1 || { tail -60 "$arena_root/.arena-dev/build.log"; exit 1; }
    tail -12 "$arena_root/.arena-dev/build.log"
    ;;
  runner)
    cc -std=c11 -O2 -DENABLE_VFS -DENABLE_DIRECTORIES -DENABLE_DEBUGGERS -DM_CORE_GBA -DM_CORE_GB \
      -I "$arena_local/mgba/include" -I "$arena_local/mgba/build/include" -I /opt/homebrew/include \
      tests/arena/runner.c -L "$arena_local/mgba/build" -Wl,-rpath,"$arena_local/mgba/build" \
      -lmgba -L /opt/homebrew/lib -lpng -lSDL2 -o "$arena_local/arena-runner"
    ;;
  play)
    exec python3 tools/arena/lab.py play
    ;;
  test)
    exec python3 tests/arena/test_lab.py
    ;;
  release)
    [[ "$(cat .arena-dev/pmd/matt_part 2>/dev/null || echo 1)" == "$matt_part" ]] || { printf '%s\n' "Sprite assets are for another part: MATT_PART=$matt_part ./tools/arena/dev.sh assets"; exit 1; }
    touch "$wild_json"
    build_background
    make -j4 CPP=cpp ARENA_LAB=0 FILE_NAME="$rel_name" BUILD_DIR="$rel_dir" WILD_JSON="$wild_json" MATT_PART="$matt_part" > "$arena_local/release-build.log" 2>&1 || { tail -60 "$arena_local/release-build.log"; exit 1; }
    tail -12 "$arena_local/release-build.log"
    ;;
  *)
    printf '%s\n' 'dev.sh assets|build|runner|test|play|release' 'python3 tools/arena/lab.py boot|status|battle|press|wait|snapshot|restore|save|screenshot'
    ;;
esac
