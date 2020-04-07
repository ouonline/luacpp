project = CreateProject()

target = project:CreateBinary("test_luacpp")
target:AddSourceFiles("*.cpp")
target:AddLibrary("..", "luacpp", STATIC)

return project
