add_rules("mode.debug", "mode.release")
add_repositories("local-repo local_repository")
set_languages("cxx17")

option("header_only", {default = true, showmenu = true, description = "Build as header only library"})

add_requires("opencv", "libnest2d")
add_requires("supercell_core")

target("atlas_generator")
    set_kind("headeronly")

    add_packages("opencv", "libnest2d", "supercell_core")

    if not has_config("header_only") then
        set_kind("$(kind)")
        add_files("source/**.cpp")
    end
    add_headerfiles("include/(**.h)")
    add_includedirs("include")
