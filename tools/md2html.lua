-- Bộ lọc pandoc: render khối ```plantuml thành SVG nhúng thẳng vào trang,
-- và đổi link giữa các file .md thành link .html.

local java = os.getenv("JAVA") or "java"
local jar = os.getenv("PLANTUML_JAR")

function CodeBlock(el)
  if el.classes[1] ~= "plantuml" then
    return nil
  end

  -- Ép Java xuất UTF-8: khi được pandoc gọi trên Windows, Java mặc định dùng
  -- bảng mã của hệ thống và làm hỏng chữ tiếng Việt trong SVG
  local ok, svg = pcall(pandoc.pipe, java,
    { "-Dstdout.encoding=UTF-8", "-Dfile.encoding=UTF-8",
      "-jar", jar, "-tsvg", "-pipe", "-charset", "UTF-8" }, el.text)
  if not ok then
    error("PlantUML render failed: " .. tostring(svg))
  end

  -- Bỏ dòng khai báo XML, vì SVG được nhúng vào giữa trang HTML
  svg = svg:gsub("^%s*<%?xml.-%?>", "")
  -- PlantUML thay chữ có dấu trong <title> (tooltip) bằng dấu chấm, nên bỏ hẳn
  svg = svg:gsub("<title>.-</title>", "")
  return pandoc.RawBlock("html", '<div class="diagram">' .. svg .. "</div>")
end

function Link(el)
  -- Chỉ đổi link nội bộ; giữ nguyên link web
  if not el.target:match("^%a+://") then
    el.target = el.target:gsub("%.md$", ".html"):gsub("%.md#", ".html#")
  end
  return el
end
