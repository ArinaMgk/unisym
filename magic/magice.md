# 【ZH-CN】 mgc (MAgicRK) 指令参考手册

`mgc` 宏引擎支持以下全局函数（宏指令）。在 `.mgc` 脚本文件中，这些指令可用于组织和渲染多端统一排版格式（HTML / Markdown / LaTeX / STDOUT）。

## 基础控制与排版

| 指令 | 参数说明 | 用途 |
| :--- | :--- | :--- |
| `Title(level, text)` | `level`: 标题层级（如 `"1"`、`"2"`）<br>`text`: 标题文本 | 输出标题（例如转化为 `\section` 或 `<h1>`）。 |
| `Format(text)` | `text`: 待格式化的文本 | 输出支持内联排版的文本。支持换行符 `\n` 处理，以及以下符号区间：<br> - `^B...^b` (加粗)<br> - `^I...^i` (斜体)<br> - `^U...^u` (下划线)<br> - `^M...^m` (原样/行内代码)<br> - `^T...^t` (等宽/打字机)<br> - `^^` (清除所有活动样式) |
| `Out(text)` | `text`: 文本内容 | 原样输出文本，无内部格式标签转换。 |
| `Set(var, val)` | `var`: 变量名<br>`val`: 变量值 | 设置文档的全局或局部变量。 |
| `Append(var, val)` | `var`: 变量名<br>`val`: 追加文本 | 往已有变量后追加内容。 |

## 资源引用与包含

| 指令 | 参数说明 | 用途 |
| :--- | :--- | :--- |
| `Picture(filepath)` | `filepath`: 图片相对路径 | 插入图片。 |
| `Include(filepath)` | `filepath`: `.mgc` 相对路径 | 包含并执行另一个 `.mgc` 文件，支持相对当前文件路径的自动解析，内置循环包含检查。 |
| `IncludeWeak(filepath)`| `filepath`: `.mgc` 相对路径 | 弱包含，找不到文件时只触发警告而不会报错退出。底层的实现在输出为 Tex 时会自动生成 `\input{...}` 指令。 |
| `Inline(target, text)` | `target`: 目标格式（如 `"tex"`, `"html"`, `"md"`）<br>`text`: 待输出字符串 | 靶点专用的内联输出块。只有当当前编译的目标格式匹配 `target` 时，才会将 `text` 原封不动输出并带上换行，否则忽略该指令（用来屏蔽跨格式的不兼容原生代码）。 |
| `TexSubimport(dir, file)`| `dir`: 目录路径<br>`file`: 文件名 | (特供) 生成底层的 `\subimport{dir}{file}` LaTeX 指令。仅在输出格式为 Tex 时有效。 |
| `CodeInclude(lang, filepath)` | `lang`: 代码语言（如 `"c++"`）<br>`filepath`: 源码文件相对路径 | 包含一个外部纯源码文件，并使用原生代码块环境（Raw）原汁原味地渲染输出。 |

## 表格 (Table)

| 指令 | 参数说明 | 用途 |
| :--- | :--- | :--- |
| `TableBegin(alignments)`| `alignments`: 列对齐规则 | 开启表格环境。`alignments` 形如 `"\|c\|c\|c\|"`（`c` 居中、`l` 左对齐、`r` 右对齐）。 |
| `TableEnd()` | (无) | 结束当前表格环境。 |
| `Row(rowspan)` | `rowspan`: 跨行数（可选，默认为 `"1"`） | 开启表格的普通数据新行。 |
| `RowHead(rowspan)` | `rowspan`: 跨行数（可选，默认为 `"1"`） | 开启表头行（自动处理 LaTeX 的 `\endhead` 等）。 |
| `Rcell(text)` | `text`: 单元格文本 | 填入当前行的下一个单元格（短语快捷用法）。内部支持 `Format` 的内联样式符号。 |
| `RcellBegin(colspan, align, rowspan)` | `colspan`: 跨列数<br>`align`: 对齐方式<br>`rowspan`: 跨行数 | 开启一个单元格容器，用于在单元格内部嵌套 `Math()` 等其它复杂环境。 |
| `RcellEnd()` | (无) | 结束当前的单元格容器。 |
| `RcellSkip()` | (无) | 跳过当前单元格（留下空位）。 |
| `LineH()` | (无) | 在表格行与行之间绘制水平分割线（如 LaTeX 的 `\hline`）。 |
| `TableCSV(filepath)` | `filepath`: CSV 文件相对路径 | 从外部 CSV 数据直接自动生成并填入对应的表数据。 |

## 列表 (List)

| 指令 | 参数说明 | 用途 |
| :--- | :--- | :--- |
| `ListBegin(type)` | `type`: 列表类型（如 `"itemize"` 或 `"enumerate"`） | 开启列表环境。如果不传类型，默认降级为 `"itemize"`（无序列表）。 |
| `ListEnd()` | (无) | 结束当前列表环境。 |
| `ListItem(prefix, text)` | `prefix`: 列表项前缀（可选）<br>`text`: 列表项文本 | 生成一个列表项（短语快捷用法）。内部支持 `Format` 的内联样式。 |
| `ListLiumBegin(prefix)` | `prefix`: 列表项前缀（可选） | 开启一个列表项容器，用于在项内部嵌套 `Math()` 等其它复杂环境（Lium 取自“奁”）。 |
| `ListLiumEnd()` | (无) | 结束当前的列表项容器。 |

## 文献引用 (Bibliography & Citations)

| 指令 | 参数说明 | 用途 |
| :--- | :--- | :--- |
| `Cite(id)` | `id`: 文献的唯一标识符 | 在正文中生成引用标注（例如 LaTeX 的 `\cite{id}` 或 HTML 的锚点跳转 `[id]`）。 |
| `BibBegin(label)` | `label`: 最宽标签（可选，默认 `"99"`） | 开启文献列表环境（例如 LaTeX 的 `\begin{thebibliography}{99}`）。 |
| `BibEnd()` | (无) | 结束文献列表环境。 |
| `BibItem(id, text)` | `id`: 文献的唯一标识符<br>`text`: 文献的描述内容 | 定义一条文献参考条目。必须包裹在 `BibBegin` 和 `BibEnd` 内部。 |

## 局部字体与字号控制 (Font & Size Control)

| 指令 | 参数说明 | 用途 |
| :--- | :--- | :--- |
| `FontBegin(size, blskip, family)`| `size`: 字号（如 `"6pt"`）<br>`blskip`: 行距（可选，默认同 `size`）<br>`family`: 字体族（可选） | 开启一个控制环境范围的局部字体/字号设定块（例如包裹巨大的表格）。 |
| `FontEnd()` | (无) | 结束环境级局部字体/字号设定块。 |
| `FontSize(size, text)` | `size`: 字号（如 `"6pt"`）<br>`text`: 内联文本 | 控制局部的某一段特定文本的字号。内部支持内联排版格式解析。 |
| `FontFamily(fam, text)` | `fam`: 字体族<br>`text`: 内联文本 | 控制局部的某一段特定文本的字体。内部支持内联排版格式解析。 |

## 数学符号与公式 (Math & Symbols)

| 指令 | 参数说明 | 用途 |
| :--- | :--- | :--- |
| `Math(text)` | `text`: 数学公式或符号代码 | 生成行内数学公式或特殊符号。在 LaTeX 中生成 `$text$`，在 HTML/Markdown 中生成 `\(text\)` 或 `$text$`（兼容 MathJax）。 |
| `MathBlock(text)` | `text`: 数学公式代码 | 生成块级（独立成行）的数学公式环境。在 LaTeX/HTML 中生成 `\[text\]`，在 Markdown 中生成 `$$text$$`。 |
