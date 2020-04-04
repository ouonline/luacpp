project = CreateProject()

target = project:CreateLibrary("luacpp")
target:AddSourceFile("*.cpp")
target:AddStaticLibrary("../../../lua/src", "lua")
target:AddSystemDynamicLibraries("dl", "m")

return project
