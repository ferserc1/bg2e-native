'.\deps\glfw', '.\deps\glfw.zip', '.\deps\bg2-io' |
ForEach {
    try { Remove-Item -Path $PSitem -Recurse -Force -ErrorAction Stop }
    catch {}
}
