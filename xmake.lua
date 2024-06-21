add_rules("mode.debug", "mode.release")

add_repositories("local-repo local_repository")

set_languages("cxx17")

add_requires("opencv", "libnest2d")
add_requires("supercell_core")

target("atlas_generator")
    set_kind("$(kind)")

    add_packages("opencv", "libnest2d", "supercell_core")

    add_files("source/**.cpp")
    add_headerfiles("include/(**.h)")
    add_includedirs("include")
