#!/usr/bin/env python3
import sys, re, os
rttype, basename = sys.argv[1:]

if rttype in ("nvvm", "spir"):
    # we need to patch
    result = []
    filename = basename+"."+rttype
    if os.path.isfile(filename):
        with open(filename) as f:
            for line in f:
                # patch to opaque identity functions
                if rttype=="spir":
                    m = re.match('^declare cc75 (.*) @(magic_.*_id)\((.*)\)\n$', line)
                else:
                    m = re.match('^declare (.*) @(magic_.*_id)\((.*)\)\n$', line)
                if m is not None:
                    ty1, fname, ty2 = m.groups()
                    assert ty1 == ty2, "Argument and return types of magic IDs must match"
                    print("Patching magic ID {0}".format(fname))
                    # emit definition instead
                    if rttype=="spir":
                        result.append('define cc75 {0} @{1}({0} %name) {{\n'.format(ty1, fname))
                    else:
                        result.append('define {0} @{1}({0} %name) {{\n'.format(ty1, fname))
                    result.append('  ret {0} %name\n'.format(ty1))
                    result.append('}\n')
                # get rid of attributes
                elif 'attributes #' in line:
                    print("Removing attribute declarations")
                    # ignore this line
                    pass
                # it's a normal line, keep it but apply substitutions
                else:
                    line = re.sub('#[0-9]+', '', line)
                    result.append(line)
        # we have the patched thing, write it
        with open(filename, "w") as f:
            for line in result:
                f.write(line)

if rttype in ("cuda", "opencl"):
    # we need to patch
    result = []
    if rttype == "cuda":
        filename = basename+"."+"cu"
    else:
        filename = basename+"."+"cl"
    if os.path.isfile(filename):
        with open(filename) as f:
            for line in f:
                # patch to opaque identity functions
                m = re.match('^(.*) = (magic_.*_id)\((.*)\);\n$', line)
                if m is not None:
                    lhs, fname, arg = m.groups()
                    print("Patching magic ID {0}".format(fname))
                    # emit definition instead
                    result.append('{0} = {1};\n'.format(lhs, arg))
                else:
                    result.append(line)
        # we have the patched thing, write it
        with open(filename, "w") as f:
            for line in result:
                f.write(line)

# another pass to add the ldg, minmax and consorts to the nvvm file
nvvm_defs = {
}

if rttype == "nvvm":
    result = []
    filename = basename+".nvvm"
    if os.path.isfile(filename):
        with open(filename) as f:
            for line in f:
                matched = False

                for (func, code) in nvvm_defs.iteritems():
                    m = re.match('^declare (.*) (@' + func + ')\((.*)\)\n$', line)
                    if m is not None:
                        result.append(code)
                        matched = True
                        break

                if not matched:
                    result.append(line)

        with open(filename, "w") as f:
            for line in result:
                f.write(line)

