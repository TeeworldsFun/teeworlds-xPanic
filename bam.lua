target_family = os.getenv("TARGET_FAMILY")
if target_family then
    family = target_family
end
target_platform = os.getenv("TARGET_PLATFORM")
if target_platform then
    platform = target_platform
end
target_arch = os.getenv("TARGET_ARCH")
if target_arch then
    arch = target_arch
end

Import("configure.lua")
Import("other/sdl/sdl.lua")
Import("other/freetype/freetype.lua")
Import("other/curl/curl.lua")
Import("other/opus/opusfile.lua")
Import("other/opus/opus.lua")
Import("other/opus/ogg.lua")
Import("other/mysql/mysql.lua")

--- Setup Config -------
config = NewConfig()
config:Add(OptCCompiler("compiler"))
config:Add(OptTestCompileC("stackprotector", "int main(){return 0;}", "-fstack-protector -fstack-protector-all"))
config:Add(OptTestCompileC("minmacosxsdk", "int main(){return 0;}",
    "-mmacosx-version-min=10.7 -isysroot /Developer/SDKs/MacOSX10.7.sdk"))
config:Add(OptTestCompileC("macosxppc", "int main(){return 0;}", "-arch ppc"))
config:Add(OptLibrary("zlib", "zlib.h", false))
config:Add(SDL.OptFind("sdl", true))
config:Add(FreeType.OptFind("freetype", true))
config:Add(Curl.OptFind("curl", true))
config:Add(Opusfile.OptFind("opusfile", true))
config:Add(Opus.OptFind("opus", true))
config:Add(Ogg.OptFind("ogg", true))
config:Add(Mysql.OptFind("mysql", false))
config:Add(OptString("websockets", false))
config:Finalize("config.lua")

-- data compiler
function Script(name)
    if family == "windows" and target_family ~= "windows" then
        return str_replace(name, "/", "\\")
    end
    return "python3 " .. name
end

function CHash(output, ...)
    local inputs = TableFlatten({...})

    output = Path(output)

    -- compile all the files
    local cmd = Script("scripts/cmd5.py") .. " "
    for index, inname in ipairs(inputs) do
        cmd = cmd .. Path(inname) .. " "
    end

    cmd = cmd .. " > " .. output

    AddJob(output, "cmd5 " .. output, cmd)
    for index, inname in ipairs(inputs) do
        AddDependency(output, inname)
    end
    AddDependency(output, "scripts/cmd5.py")
    return output
end

--[[
function DuplicateDirectoryStructure(orgpath, srcpath, dstpath)
	for _,v in pairs(CollectDirs(srcpath .. "/")) do
		MakeDirectory(dstpath .. "/" .. string.sub(v, string.len(orgpath)+2))
		DuplicateDirectoryStructure(orgpath, v, dstpath)
	end
end

DuplicateDirectoryStructure("src", "src", "objs")
]]

function ResCompile(scriptfile)
    windres = os.getenv("WINDRES")
    if not windres then
        windres = "windres"
    end

    scriptfile = Path(scriptfile)
    if config.compiler.driver == "cl" then
        output = PathBase(scriptfile) .. ".res"
        AddJob(output, "rc " .. scriptfile, "rc /fo " .. output .. " " .. scriptfile)
    elseif config.compiler.driver == "gcc" then
        output = PathBase(scriptfile) .. ".coff"
        AddJob(output, windres .. " " .. scriptfile, windres .. " -i " .. scriptfile .. " -o " .. output)
    end

    AddDependency(output, scriptfile)
    return output
end

function Dat2c(datafile, sourcefile, arrayname)
    datafile = Path(datafile)
    sourcefile = Path(sourcefile)

    AddJob(sourcefile, "dat2c " .. PathFilename(sourcefile) .. " = " .. PathFilename(datafile),
        Script("scripts/dat2c.py") .. "\" " .. sourcefile .. " " .. datafile .. " " .. arrayname)
    AddDependency(sourcefile, datafile)
    return sourcefile
end

function ContentCompile(action, output)
    output = Path(output)
    AddJob(output, action .. " > " .. output, -- Script("datasrc/compile.py") .. "\" ".. Path(output) .. " " .. action
    Script("datasrc/compile.py") .. " " .. action .. " > " .. Path(output))
    AddDependency(output, Path("datasrc/content.py")) -- do this more proper
    AddDependency(output, Path("datasrc/network.py"))
    AddDependency(output, Path("datasrc/compile.py"))
    AddDependency(output, Path("datasrc/datatypes.py"))
    return output
end

-- Content Compile
network_source = ContentCompile("network_source", "src/game/generated/protocol.cpp")
network_header = ContentCompile("network_header", "src/game/generated/protocol.h")
server_content_source = ContentCompile("server_content_source", "src/game/generated/server_data.cpp")
server_content_header = ContentCompile("server_content_header", "src/game/generated/server_data.h")

AddDependency(network_source, network_header)
AddDependency(server_content_source, server_content_header)

nethash = CHash("src/game/generated/nethash.cpp", "src/engine/shared/protocol.h", "src/game/generated/protocol.h",
    "src/game/tuning.h", "src/game/gamecore.cpp", network_header)

icu_depends = {}
server_link_other = {}
server_sql_depends = {}

if family == "windows" then
    if platform == "win32" then
        if config.compiler.driver == "cl" then
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib32/icudt53.dll"))
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib32/icuin53.dll"))
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib32/icuuc53.dll"))
        elseif config.compiler.driver == "gcc" then
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib32/icudt53.dll"))
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib32/icuin53.dll"))
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib32/icuuc53.dll"))
        end
    else
        -- Add ICU because its a HAVE to
        if config.compiler.driver == "cl" then
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib64/icudt53.dll"))
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib64/icuin53.dll"))
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/vc/lib64/icuuc53.dll"))
        elseif config.compiler.driver == "gcc" then
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib64/icudt53.dll"))
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib64/icuin53.dll"))
            table.insert(icu_depends, CopyToDirectory(".", "other/icu/gcc/lib64/icuuc53.dll"))
        end
    end
    table.insert(server_sql_depends, CopyToDirectory(".", "other/mysql/vc2005libs/mysqlcppconn.dll"))
    table.insert(server_sql_depends, CopyToDirectory(".", "other/mysql/vc2005libs/libmysql.dll"))

    if config.compiler.driver == "cl" then
        server_link_other = {ResCompile("other/icons/teeworlds_srv_cl.rc")}
    elseif config.compiler.driver == "gcc" then
        server_link_other = {ResCompile("other/icons/teeworlds_srv_gcc.rc")}
    end
end

function Intermediate_Output(settings, input)
    return "objs/" .. string.sub(PathBase(input), string.len("src/") + 1) .. settings.config_ext
end

function build(settings)
    -- apply compiler settings
    config.compiler:Apply(settings)

    -- settings.objdir = Path("objs")
    settings.cc.Output = Intermediate_Output

    cc = os.getenv("CC")
    if cc then
        settings.cc.exe_c = cc
    end
    cxx = os.getenv("CXX")
    if cxx then
        settings.cc.exe_cxx = cxx
        settings.link.exe = cxx
        settings.dll.exe = cxx
    end
    cflags = os.getenv("CFLAGS")
    if cflags then
        settings.cc.flags:Add(cflags)
    end
    ldflags = os.getenv("LDFLAGS")
    if ldflags then
        settings.link.flags:Add(ldflags)
    end

    if config.websockets.value then
        settings.cc.defines:Add("WEBSOCKETS")
    end

    if config.compiler.driver == "cl" then
        settings.cc.flags:Add("/wd4244")
        settings.cc.flags:Add("/EHsc")
    else
        settings.cc.flags:Add("-Wall")
        if family == "windows" then
            if config.compiler.driver == "gcc" then
                settings.link.flags:Add("-static-libgcc")
                settings.link.flags:Add("-static-libstdc++")
            end
            -- disable visibility attribute support for gcc on windows
            settings.cc.defines:Add("NO_VIZ")
        elseif platform == "macosx" then
            settings.cc.flags:Add("-mmacosx-version-min=10.7")
            settings.link.flags:Add("-mmacosx-version-min=10.7")
            settings.cc.flags:Add("-stdlib=libc++")
            settings.link.flags:Add("-stdlib=libc++")
            if config.minmacosxsdk.value == 1 then
                settings.cc.flags:Add("-isysroot /Developer/SDKs/MacOSX10.7.sdk")
                settings.link.flags:Add("-isysroot /Developer/SDKs/MacOSX10.7.sdk")
            end
        elseif config.stackprotector.value == 1 then
            settings.cc.flags:Add("-fstack-protector", "-fstack-protector-all")
            settings.link.flags:Add("-fstack-protector", "-fstack-protector-all")
        end
    end

    settings.cc.includes:Add("src")
    settings.cc.includes:Add("src/engine/external")

    -- set some platform specific settings
    if family == "unix" then
        if platform == "macosx" then
            settings.cc.flags_cxx:Add("-stdlib=libc++")
            settings.cc.includes:Add("/usr/local/opt/icu4c/include")
            settings.link.libs:Add("icui18n")
            settings.link.libs:Add("icuuc")
            settings.link.libs:Add("c++")
            settings.link.libpath:Add("/usr/local/opt/icu4c/lib")
            settings.link.frameworks:Add("Carbon")
            settings.link.frameworks:Add("AppKit")
        else
            settings.link.libs:Add("pthread")
            -- add ICU for linux
            if ExecuteSilent("pkg-config icu-uc icu-i18n") == 0 then
            end

            settings.cc.flags:Add("`pkg-config --cflags icu-uc icu-i18n`")
            settings.link.flags:Add("`pkg-config --libs icu-uc icu-i18n`")
        end

        if platform == "solaris" then
            settings.link.flags:Add("-lsocket")
            settings.link.flags:Add("-lnsl")
        end
    elseif family == "windows" then
        settings.link.libs:Add("gdi32")
        settings.link.libs:Add("user32")
        settings.link.libs:Add("ws2_32")
        settings.link.libs:Add("ole32")
        settings.link.libs:Add("shell32")
        settings.link.libs:Add("advapi32")

        -- add ICU also here
        settings.cc.includes:Add("other\\icu\\include")
    end

    -- compile zlib if needed
    if config.zlib.value == 1 then
        settings.link.libs:Add("z")
        if config.zlib.include_path then
            settings.cc.includes:Add(config.zlib.include_path)
        end
        zlib = {}
    else
        zlib = Compile(settings, Collect("src/engine/external/zlib/*.c"))
        settings.cc.includes:Add("src/engine/external/zlib")
    end

    -- build the small libraries
    wavpack = Compile(settings, Collect("src/engine/external/wavpack/*.c"))
    pnglite = Compile(settings, Collect("src/engine/external/pnglite/*.c"))
    jsonparser = Compile(settings, Collect("src/engine/external/json-parser/*.c"))
    md5 = Compile(settings, "src/engine/external/md5/md5.c")
    if config.websockets.value then
        libwebsockets = Compile(settings, Collect("src/engine/external/libwebsockets/*.c"))
    end

    -- build game components
    engine_settings = settings:Copy()
    server_settings = engine_settings:Copy()
    launcher_settings = engine_settings:Copy()

    if family == "windows" then
        if string.find(settings.config_name, "sql") then
            server_settings.link.libpath:Add("other/mysql/vc2005libs")
            server_settings.link.libs:Add("mysqlcppconn")
        end

        -- Add ICU because its a HAVE to
        if platform == "win32" then
            if config.compiler.driver == "cl" then
                server_settings.link.libpath:Add("other/icu/vc/lib32")
            elseif config.compiler.driver == "gcc" then
                server_settings.link.libpath:Add("other/icu/gcc/lib32")
            end
            server_settings.link.libs:Add("icudt")
            server_settings.link.libs:Add("icuin")
            server_settings.link.libs:Add("icuuc")
        else
            if config.compiler.driver == "cl" then
                server_settings.link.libpath:Add("other/icu/vc/lib64")
            elseif config.compiler.driver == "gcc" then
                server_settings.link.libpath:Add("other/icu/gcc/lib64")
            end
            server_settings.link.libs:Add("icudt")
            server_settings.link.libs:Add("icuin")
            server_settings.link.libs:Add("icuuc")
        end
    end

    if family == "unix" and (platform == "macosx" or platform == "linux") then
        engine_settings.link.libs:Add("dl")
        server_settings.link.libs:Add("dl")
        launcher_settings.link.libs:Add("dl")
    end

    engine = Compile(engine_settings, Collect("src/engine/shared/*.cpp", "src/base/*.c"))
    server = Compile(server_settings, Collect("src/engine/server/*.cpp"))
    teeothers = Compile(server_settings,
        Collect("src/teeothers/*.cpp", "src/teeothers/components/*.cpp", "src/teeothers/system/*.cpp"))

    versionserver = Compile(settings, Collect("src/versionsrv/*.cpp"))
    masterserver = Compile(settings, Collect("src/mastersrv/*.cpp"))
    twping = Compile(settings, Collect("src/twping/*.cpp"))
    game_shared = Compile(settings, Collect("src/game/*.cpp"), nethash, network_source)
    game_server = Compile(settings, CollectRecursive("src/game/server/*.cpp"), server_content_source)

    -- build tools (TODO: fix this so we don't get double _d_d stuff)
    tools_src = Collect("src/tools/*.cpp", "src/tools/*.c")

    server_osxlaunch = {}
    if platform == "macosx" then
        notification_settings.cc.flags:Add("-x objective-c++")
        server_osxlaunch = Compile(launcher_settings, "src/osxlaunch/server.m")
    end

    tools = {}
    for i, v in ipairs(tools_src) do
        toolname = PathFilename(PathBase(v))
        tools[i] = Link(settings, toolname, Compile(settings, v), engine, zlib, pnglite, md5)
    end

    server_exe = Link(server_settings, "xPanic-Server", engine, server, game_shared, game_server, zlib,
        server_link_other, libwebsockets, md5, icu_depends, teeothers, jsonparser)

    serverlaunch = {}
    if platform == "macosx" then
        serverlaunch = Link(launcher_settings, "serverlaunch", server_osxlaunch)
    end

    versionserver_exe = Link(server_settings, "versionsrv", versionserver, engine, zlib, libwebsockets, md5)

    masterserver_exe = Link(server_settings, "mastersrv", masterserver, engine, zlib, md5)

    twping_exe = Link(server_settings, "twping", twping, engine, zlib, md5)

    -- make targets
    if string.find(settings.config_name, "sql") then
        s = PseudoTarget("server" .. "_" .. settings.config_name, server_exe, serverlaunch, server_sql_depends)
    else
        s = PseudoTarget("server" .. "_" .. settings.config_name, server_exe, serverlaunch)
    end
    g = PseudoTarget("game" .. "_" .. settings.config_name, server_exe)

    v = PseudoTarget("versionserver" .. "_" .. settings.config_name, versionserver_exe)
    m = PseudoTarget("masterserver" .. "_" .. settings.config_name, masterserver_exe)
    t = PseudoTarget("tools" .. "_" .. settings.config_name, tools)
    p = PseudoTarget("twping" .. "_" .. settings.config_name, twping_exe)

    all = PseudoTarget(settings.config_name, c, s, v, m, t, p)
    return all
end

debug_settings = NewSettings()
debug_settings.config_name = "debug"
debug_settings.config_ext = "_d"
debug_settings.debug = 1
debug_settings.optimize = 0
debug_settings.cc.defines:Add("CONF_DEBUG")

debug_sql_settings = NewSettings()
debug_sql_settings.config_name = "sql_debug"
debug_sql_settings.config_ext = "_sql_d"
debug_sql_settings.debug = 1
debug_sql_settings.optimize = 0
debug_sql_settings.cc.defines:Add("CONF_DEBUG", "CONF_SQL")

release_settings = NewSettings()
release_settings.config_name = "release"
release_settings.config_ext = ""
release_settings.debug = 0
release_settings.optimize = 1
release_settings.cc.defines:Add("CONF_RELEASE")

release_sql_settings = NewSettings()
release_sql_settings.config_name = "sql_release"
release_sql_settings.config_ext = "_sql"
release_sql_settings.debug = 0
release_sql_settings.optimize = 1
release_sql_settings.cc.defines:Add("CONF_RELEASE", "CONF_SQL")

config.mysql:Apply(debug_sql_settings)
config.mysql:Apply(release_sql_settings)

if platform == "macosx" then
    debug_settings_ppc = debug_settings:Copy()
    debug_settings_ppc.config_name = "debug_ppc"
    debug_settings_ppc.config_ext = "_ppc_d"
    debug_settings_ppc.cc.flags:Add("-arch ppc")
    debug_settings_ppc.link.flags:Add("-arch ppc")
    debug_settings_ppc.cc.defines:Add("CONF_DEBUG")

    debug_sql_settings_ppc = debug_sql_settings:Copy()
    debug_sql_settings_ppc.config_name = "sql_debug_ppc"
    debug_sql_settings_ppc.config_ext = "_sql_ppc_d"
    debug_sql_settings_ppc.cc.flags:Add("-arch ppc")
    debug_sql_settings_ppc.link.flags:Add("-arch ppc")
    debug_sql_settings_ppc.cc.defines:Add("CONF_DEBUG", "CONF_SQL")

    release_settings_ppc = release_settings:Copy()
    release_settings_ppc.config_name = "release_ppc"
    release_settings_ppc.config_ext = "_ppc"
    release_settings_ppc.cc.flags:Add("-arch ppc")
    release_settings_ppc.link.flags:Add("-arch ppc")
    release_settings_ppc.cc.defines:Add("CONF_RELEASE")

    release_sql_settings_ppc = release_sql_settings:Copy()
    release_sql_settings_ppc.config_name = "sql_release_ppc"
    release_sql_settings_ppc.config_ext = "_sql_ppc"
    release_sql_settings_ppc.cc.flags:Add("-arch ppc")
    release_sql_settings_ppc.link.flags:Add("-arch ppc")
    release_sql_settings_ppc.cc.defines:Add("CONF_RELEASE", "CONF_SQL")

    ppc_d = build(debug_settings_ppc)
    ppc_r = build(release_settings_ppc)
    sql_ppc_d = build(debug_sql_settings_ppc)
    sql_ppc_r = build(release_sql_settings_ppc)

    if arch == "ia32" or arch == "amd64" then
        debug_settings_x86 = debug_settings:Copy()
        debug_settings_x86.config_name = "debug_x86"
        debug_settings_x86.config_ext = "_x86_d"
        debug_settings_x86.cc.flags:Add("-arch i386")
        debug_settings_x86.link.flags:Add("-arch i386")
        debug_settings_x86.cc.defines:Add("CONF_DEBUG")

        debug_sql_settings_x86 = debug_sql_settings:Copy()
        debug_sql_settings_x86.config_name = "sql_debug_x86"
        debug_sql_settings_x86.config_ext = "_sql_x86_d"
        debug_sql_settings_x86.cc.flags:Add("-arch i386")
        debug_sql_settings_x86.link.flags:Add("-arch i386")
        debug_sql_settings_x86.cc.defines:Add("CONF_DEBUG", "CONF_SQL")

        release_settings_x86 = release_settings:Copy()
        release_settings_x86.config_name = "release_x86"
        release_settings_x86.config_ext = "_x86"
        release_settings_x86.cc.flags:Add("-arch i386")
        release_settings_x86.link.flags:Add("-arch i386")
        release_settings_x86.cc.defines:Add("CONF_RELEASE")

        release_sql_settings_x86 = release_sql_settings:Copy()
        release_sql_settings_x86.config_name = "sql_release_x86"
        release_sql_settings_x86.config_ext = "_sql_x86"
        release_sql_settings_x86.cc.flags:Add("-arch i386")
        release_sql_settings_x86.link.flags:Add("-arch i386")
        release_sql_settings_x86.cc.defines:Add("CONF_RELEASE", "CONF_SQL")

        x86_d = build(debug_settings_x86)
        sql_x86_d = build(debug_sql_settings_x86)
        x86_r = build(release_settings_x86)
        sql_x86_r = build(release_sql_settings_x86)
    end

    if arch == "amd64" then
        debug_settings_x86_64 = debug_settings:Copy()
        debug_settings_x86_64.config_name = "debug_x86_64"
        debug_settings_x86_64.config_ext = "_x86_64_d"
        debug_settings_x86_64.cc.flags:Add("-arch x86_64")
        debug_settings_x86_64.link.flags:Add("-arch x86_64")
        debug_settings_x86_64.cc.defines:Add("CONF_DEBUG")

        debug_sql_settings_x86_64 = debug_sql_settings:Copy()
        debug_sql_settings_x86_64.config_name = "sql_debug_x86_64"
        debug_sql_settings_x86_64.config_ext = "_sql_x86_64_d"
        debug_sql_settings_x86_64.cc.flags:Add("-arch x86_64")
        debug_sql_settings_x86_64.link.flags:Add("-arch x86_64")
        debug_sql_settings_x86_64.cc.defines:Add("CONF_DEBUG", "CONF_SQL")

        release_settings_x86_64 = release_settings:Copy()
        release_settings_x86_64.config_name = "release_x86_64"
        release_settings_x86_64.config_ext = "_x86_64"
        release_settings_x86_64.cc.flags:Add("-arch x86_64")
        release_settings_x86_64.link.flags:Add("-arch x86_64")
        release_settings_x86_64.cc.defines:Add("CONF_RELEASE")

        release_sql_settings_x86_64 = release_sql_settings:Copy()
        release_sql_settings_x86_64.config_name = "sql_release_x86_64"
        release_sql_settings_x86_64.config_ext = "_sql_x86_64"
        release_sql_settings_x86_64.cc.flags:Add("-arch x86_64")
        release_sql_settings_x86_64.link.flags:Add("-arch x86_64")
        release_sql_settings_x86_64.cc.defines:Add("CONF_RELEASE", "CONF_SQL")

        x86_64_d = build(debug_settings_x86_64)
        sql_x86_64_d = build(debug_sql_settings_x86_64)
        x86_64_r = build(release_settings_x86_64)
        sql_x86_64_r = build(release_sql_settings_x86_64)
    end

    DefaultTarget("game_debug_x86")

    if config.macosxppc.value == 1 then
        if arch == "ia32" then
            PseudoTarget("release", ppc_r, x86_r)
            PseudoTarget("debug", ppc_d, x86_d)
            PseudoTarget("server_release", "server_release_ppc", "server_release_x86")
            PseudoTarget("server_debug", "server_debug_ppc", "server_debug_x86")
            PseudoTarget("sql_release", sql_ppc_r, sql_x86_r)
            PseudoTarget("sql_debug", sql_ppc_d, sql_x86_d)
            PseudoTarget("server_sql_release", "server_sql_release_ppc", "server_sql_release_x86")
            PseudoTarget("server_sql_debug", "server_sql_debug_ppc", "server_sql_debug_x86")
        elseif arch == "amd64" then
            PseudoTarget("release", ppc_r, x86_r, x86_64_r)
            PseudoTarget("debug", ppc_d, x86_d, x86_64_d)
            PseudoTarget("server_release", "server_release_ppc", "server_release_x86", "server_release_x86_64")
            PseudoTarget("server_debug", "server_debug_ppc", "server_debug_x86", "server_debug_x86_64")
            PseudoTarget("sql_release", sql_ppc_r, sql_x86_r, sql_x86_64_r)
            PseudoTarget("sql_debug", sql_ppc_d, sql_x86_d, sql_x86_64_d)
            PseudoTarget("server_sql_release", "server_sql_release_ppc", "server_sql_release_x86",
                "server_sql_release_x86_64")
            PseudoTarget("server_sql_debug", "server_sql_debug_ppc", "server_sql_debug_x86", "server_sql_debug_x86_64")
        else
            PseudoTarget("release", ppc_r)
            PseudoTarget("debug", ppc_d)
            PseudoTarget("server_release", "server_release_ppc")
            PseudoTarget("server_debug", "server_debug_ppc")
            PseudoTarget("sql_release", sql_ppc_r)
            PseudoTarget("sql_debug", sql_ppc_d)
            PseudoTarget("server_sql_release", "server_sql_release_ppc")
            PseudoTarget("server_sql_debug", "server_sql_debug_ppc")
        end
    else
        if arch == "ia32" then
            PseudoTarget("release", x86_r)
            PseudoTarget("debug", x86_d)
            PseudoTarget("server_release", "server_release_x86")
            PseudoTarget("server_debug", "server_debug_x86")
            PseudoTarget("sql_release", sql_x86_r)
            PseudoTarget("sql_debug", sql_x86_d)
            PseudoTarget("server_sql_release", "server_sql_release_x86")
            PseudoTarget("server_sql_debug", "server_sql_debug_x86")
        elseif arch == "amd64" then
            PseudoTarget("release", x86_r, x86_64_r)
            PseudoTarget("debug", x86_d, x86_64_d)
            PseudoTarget("server_release", "server_release_x86", "server_release_x86_64")
            PseudoTarget("server_debug", "server_debug_x86", "server_debug_x86_64")
            PseudoTarget("sql_release", sql_x86_r, sql_x86_64_r)
            PseudoTarget("sql_debug", sql_x86_d, sql_x86_64_d)
            PseudoTarget("server_sql_release", "server_sql_release_x86", "server_sql_release_x86_64")
            PseudoTarget("server_sql_debug", "server_sql_debug_x86", "server_sql_debug_x86_64")
        end
    end
else
    build(debug_settings)
    build(debug_sql_settings)
    build(release_settings)
    build(release_sql_settings)
    DefaultTarget("game_debug")
end
