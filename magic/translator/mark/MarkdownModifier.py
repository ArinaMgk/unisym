# encoding="utf-8" author="ArinaMgk" date="RFX-03" language="script/Python"
# Easy conversion from Obsidian to wider-used Markdown format
import os

prefix_dir = "E:/PROJ/SVGN/HerNote/"

# Get the list of Markdown files
def get_markdown_files():
    mdfiles = []
    for root, dirs, files in os.walk("."):
        for filename in files:
            if filename.endswith('.md'):
                mdfiles.append(os.path.join(root, filename))
    return mdfiles

# Convert Hernote-relatived path to Drive-relatived path in Markdown files
def convert_image_path(filepath, suffix):
    """
    ![[_resources/Pasted image 20230729222029.png|512]]
    ![](P:/_resources/Pasted image 20230729222029.png)
    
    SiYuan accept CRLF and CRLFCRLF
    """

    with open(filepath, 'r', encoding='utf-8') as file:
        lines = file.readlines()
        result = []
        for line in lines:
            tmpstr = line.lstrip()
            if (len(line) > 2 and line[0:2] == "# ") or (len(line) > 3 and line[0:3] == "## ") or (len(line) > 4 and line[0:4] == "### ") or (len(line) > 5 and line[0:5] == "#### ") or (len(line) > 6 and line[0:6] == "##### ") or (len(line) > 7 and line[0:7]) == "###### ": # title
                tmpstr = tmpstr + "\n"
            elif "</" in tmpstr and ">" in tmpstr: # html
                pass
            elif tmpstr and (tmpstr[0] != '|'): # table
                tmpstr = line.replace("[[_resources/", "[]({}_resources/".format(prefix_dir))
                while "|" in tmpstr and "]]" in tmpstr:
                    tmptmp = tmpstr[tmpstr.index("|") + 1:]
                    if "]]" not in tmptmp:
                        break
                    idx0 = tmpstr.index("|")
                    idx1 = tmptmp.index("]]")
              
                    tmpstr = tmpstr[:idx0] + tmptmp[idx1:]
                tmpstr = tmpstr.replace(".png]]", ".png)")
                # tmpstr = tmpstr.replace("]]", ")")
                
            result.append(tmpstr)
    with open(filepath + suffix, 'w', encoding='utf-8') as file:
        for element in result:          
            file.write(element)
    return result

if __name__ == '__main__':
    num = 0
    # convert_image_path("a.txt", ".txt")
    files = get_markdown_files()
    for file in files:
        print(str(num) + "> " + file)
        num += 1
        convert_image_path(file, "")
        #{subprocess::} subprocess.call(["pandoc", file, "-o", file.replace(".md", ".html")])
        #$ pandoc xxx.md --pdf-engine=xelatex -o xxx.pdf -V "mainfont=Microsoft YaHei"
