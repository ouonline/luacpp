project = CreateProject()

target = project:CreateLibrary("luacpp", STATIC | SHARED)
target:AddSourceFiles("*.cpp")
target:AddLibrary("../../../lua/src", "lua", STATIC)
target:AddSysLibraries("dl", "m")

return project
