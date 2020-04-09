project = CreateProject()

dep = project:CreateDependency()
dep:AddSourceFiles("*.cpp")
dep:AddFlags("-Wall", "-Werror", "-Wextra", "-fPIC")
dep:AddStaticLibrary("../../../lua/src", "lua")
dep:AddSysLibraries("dl", "m")

a = project:CreateStaticLibrary("luacpp_static")
a:AddDependencies(dep)

so = project:CreateSharedLibrary("luacpp_shared")
so:AddDependencies(dep)

return project
