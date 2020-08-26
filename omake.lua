project = Project()

dep = project:CreateDependency()
    :AddSourceFiles("*.cpp")
    :AddFlags({"-Wall", "-Werror", "-Wextra", "-fPIC"})
    :AddStaticLibraries("../../../lua/src", "lua")

project:CreateStaticLibrary("luacpp_static"):AddDependencies(dep)
project:CreateSharedLibrary("luacpp_shared"):AddDependencies(dep)

return project
