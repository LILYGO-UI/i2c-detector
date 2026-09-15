# Application icon / 应用图标

- `app-icon.svg` — editable vector master, with a `512 × 512` viewBox. All artwork uses SVG shapes; no fonts, embedded bitmaps, or external resources are required.
- `app-icon.png` — `128 × 128` RGBA export used by `lpm.toml` and the launcher. Export this file from the SVG after editing, preserving transparency outside the rounded tile.

The chip and its pins represent the I2C hardware interface. The small matrix echoes
the application's address map, while the magnifying glass and green check indicate
a detected address. Emerald `#167C5A` matches the UI's detection highlight; charcoal
`#20262D` matches its action color.

SVG 为图标的可编辑存档源文件；PNG 为启动器使用的导出文件。图形以芯片、地址矩阵和
检测放大镜表达 I2C 总线扫描，沿用界面的绿色检测高亮。修改图标时先编辑 SVG，再导出
128 × 128 PNG，并检查 32、64 和 128 像素下的辨识度。
