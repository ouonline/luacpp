project = Project()

project:CreateBinary("test_luacpp"):AddDependencies(
    project:CreateDependency()
        :AddSourceFiles("*.cpp")
        :AddFlags({"-Wall", "-Werror", "-Wextra"})
        :AddStaticLibraries("..", "luacpp_static"))

return project
