project = CreateProject()

dep = project:CreateDependency()
dep:AddSourceFiles("*.cpp")
dep:AddFlags("-Wall", "-Werror", "-Wextra")
dep:AddStaticLibrary("..", "luacpp_static")

target = project:CreateBinary("test_luacpp")
target:AddDependencies(dep)

return project
