project = CreateProject()

target = project:CreateBinary("test_luacpp")
target:AddSourceFiles("*.cpp")
target:AddFlags("-Wall", "-Werror", "-Wextra", "-fPIC")
target:AddStaticLibrary("..", "luacpp_static")

return project
