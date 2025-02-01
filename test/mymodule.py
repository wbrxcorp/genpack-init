import genpack_init

def configure(ini):
    print("Called. foo=%s" % ini.get("_default", "foo", fallback="baz"))
    genpack_init.coldplug()
    coldplug()
