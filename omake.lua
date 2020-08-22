project = CreateProject()

dep = project:CreateDependency()
    :AddSourceFiles("*.cpp")
    :AddFlags("-Wall", "-Werror", "-Wextra", "-fPIC")
    :AddStaticLibrary("../../../lua/src", "lua")

project:CreateStaticLibrary("luacpp_static"):AddDependencies(dep)
project:CreateSharedLibrary("luacpp_shared"):AddDependencies(dep)

return project
