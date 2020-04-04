project = CreateProject()

target = project:CreateBinary("test_luacpp")
target:AddSourceFile("*.cpp")
target:AddStaticLibrary("..", "luacpp")

return project
