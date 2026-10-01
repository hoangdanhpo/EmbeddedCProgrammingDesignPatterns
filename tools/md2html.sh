#!/usr/bin/env bash
# Chuyển LECTURE.md và README.md của mọi bài học sang HTML.
# Sơ đồ PlantUML được render thành SVG và nhúng thẳng vào trang, CSS cũng được nhúng,
# nên mỗi file .html mở độc lập được, không cần internet.
#
# Dùng: bash tools/md2html.sh            (mọi bài)
#       bash tools/md2html.sh 02_Opaque*  (chỉ các thư mục khớp)

set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"

pandoc_bin="${PANDOC:-}"
if [ -z "$pandoc_bin" ]; then
	if command -v pandoc >/dev/null 2>&1; then
		pandoc_bin="pandoc"
	else
		pandoc_bin="$LOCALAPPDATA/Pandoc/pandoc.exe"
	fi
fi

if [ -z "${JAVA:-}" ]; then
	if command -v java >/dev/null 2>&1; then
		JAVA="java"
	else
		JAVA="$(ls -d /c/Program\ Files/Microsoft/jdk-*/bin/java.exe 2>/dev/null | head -1)"
	fi
fi
export JAVA
export PLANTUML_JAR="${PLANTUML_JAR:-$HOME/tools/plantuml/plantuml.jar}"

# pandoc là chương trình Windows, cần đường dẫn dạng Windows
winpath() {
	if command -v cygpath >/dev/null 2>&1; then cygpath -w "$1"; else echo "$1"; fi
}
export PLANTUML_JAR="$(winpath "$PLANTUML_JAR")"

css="$(winpath "$root/tools/lecture.css")"
filter="$(winpath "$root/tools/md2html.lua")"

cd "$root"
patterns=("$@")
[ ${#patterns[@]} -eq 0 ] && patterns=("[0-9][0-9]_*")

for dir in ${patterns[@]}; do
	[ -d "$dir" ] || continue
	for md in "$dir"/LECTURE.md "$dir"/README.md; do
		[ -f "$md" ] || continue
		html="${md%.md}.html"
		# Lấy tiêu đề H1 đầu tiên làm tiêu đề tab trình duyệt
		title="$(grep -m1 '^# ' "$md" | sed 's/^# *//')"
		"$pandoc_bin" "$md" \
			--from gfm --to html5 --standalone --embed-resources \
			--css "$css" --lua-filter "$filter" \
			--metadata pagetitle="$title" --metadata lang=vi \
			--output "$html"
		echo "  $md -> $html"
	done
done
