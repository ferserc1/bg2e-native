# Download GLFW
Invoke-WebRequest -Uri "https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip" -OutFile deps\glfw.zip
Expand-Archive .\deps\glfw.zip -DestinationPath deps
Rename-Item -Path .\deps\glfw-3.3.8.bin.WIN64 -NewName glfw
Remove-Item ".\deps\glfw.zip"

# Download bg2-io
git clone https://github.com/ferserc1/bg2-io deps/bg2-io


