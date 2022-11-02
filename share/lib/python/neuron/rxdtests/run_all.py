def test(files, correct_data):
    import os
    import sys
    import filecmp
    import subprocess
    import re, array, numpy

    tol = 1e-10
    dt_eps = 1e-20
    for dr in ["wave1d", "ecs", "3d", "hybrid"]:
        try:
            os.makedirs(os.path.join("test_data", dr))
        except OSError:
            pass

    passed_list = []
    failed_list = []
    incomplete_list = []

    for f in files:
        base_name = f[:-3]
        print("%s: " % base_name)

        output_file = os.path.join("test_data", base_name + ".dat")
        # remove any old test results
        try:
            os.remove(output_file)
        except OSError:
            pass

        #        os.system('python do_test.py %s %s' % (os.path.join('tests', f), output_file))

        env = os.environ.copy()
        try:
            env[env['NRN_SANITIZER_PRELOAD_VAR']] = env['NRN_SANITIZER_PRELOAD_VAL']
        except:
            pass

        try:
            outp = subprocess.check_output(
                [sys.executable, "do_test.py", os.path.join("tests", f), output_file], env=env, shell=False,
            )
            sobj = re.search(r"<BAS_RL (\d*) BAS_RL>", outp.decode("utf-8"), re.M)
            rlen = int(sobj.group(1))
            success = False
            corr_dat = numpy.fromfile(
                os.path.join(correct_data, base_name + ".dat")
            ).reshape(-1, rlen)
            tst_dat = numpy.fromfile(output_file).reshape(-1, rlen)
            t1 = corr_dat[:, 0]
            t2 = tst_dat[:, 0]
            # remove any initial t that are greter than the next t (removes times before 0) in correct data
            c = 0
            while c < len(t1) - 1 and t1[c] > t1[c + 1]:
                c = c + 1
            t1 = numpy.delete(t1, range(c))
            corr_dat = numpy.delete(corr_dat, range(c), 0)
            # remove any initial t that are greter than the next t (removes times before 0) in test data
            c = 0
            while c < len(t2) - 1 and t2[c] > t2[c + 1]:
                c = c + 1
            t2 = numpy.delete(t2, range(c))
            tst_dat = numpy.delete(tst_dat, range(c), 0)
            # get rid of repeating t in correct data (otherwise interpolation fails)
            c = 0
            while c < len(t1) - 1:
                c1 = c + 1
                while c1 < len(t1) and abs(t1[c] - t1[c1]) < dt_eps:
                    c1 = c1 + 1
                t1 = numpy.delete(t1, range(c, c1 - 1))
                corr_dat = numpy.delete(corr_dat, range(c, c1 - 1), 0)
                c = c + 1
            # get rid of the test data outside of the correct data time interval
            t2_n = len(t2)
            t2_0 = 0
            while t2[t2_n - 1] > t1[-1]:
                t2_n = t2_n - 1
            while t2[t2_0] < t1[0]:
                t2_0 = t2_0 + 1
            # interpolate and compare
            corr_vals = numpy.array(
                [
                    numpy.interp(t2[t2_0:t2_n], t1, corr_dat[:, i].T)
                    for i in range(1, rlen)
                ]
            )
            max_err = numpy.amax(abs(corr_vals.T - tst_dat[t2_0:t2_n, 1:]))

            if max_err < tol:
                success = True

            if success:
                passed_list.append(base_name)
                print("passed")
            else:
                print("failed")

                failed_list.append(base_name)
        except Exception as e:
            print("incomplete", e)
            incomplete_list.append(base_name)

    print("")
    print("---")
    print("Passed:     ", ", ".join(passed_list))
    print("Failed:     ", ", ".join(failed_list))
    print("Incomplete: ", ", ".join(incomplete_list))
    print("---")
    print("Ran %d tests:" % len(files))
    print("Passed:     %d" % len(passed_list))
    print("Failed:     %d" % len(failed_list))
    print("Incomplete: %d" % len(incomplete_list))
    if len(passed_list) < len(files):
        return -1
    return 0


if __name__ == "__main__":
    import os
    import sys
    import shutil
    import subprocess
    import platform

    abspath = os.path.abspath(__file__)
    dname = os.path.dirname(abspath)
    os.chdir(dname)

    rxd_data = os.path.join(
        os.pardir, os.pardir, os.pardir, os.pardir, os.pardir, "test", "rxd", "testdata"
    )
    # get the rxd test data
    subprocess.call(["git", "submodule", "update", "--init", "--recursive", rxd_data])
    correct_data = os.path.abspath(os.path.join(rxd_data, "rxdtests"))

    files = [f for f in os.listdir("tests") if f[-3:].lower() == ".py"]
    # dirs = [name for name in os.listdir('tests') if os.path.isdir(os.path.join('tests', name))]
    dirs = [
        os.path.join(*d.split(os.path.sep)[1:])
        for d in [wlk[0] for wlk in os.walk("tests")][1:]
    ]
    os.chdir(dname)

    for dr in dirs:
        fname = os.path.join("tests", dr, "torun.txt")
        if os.path.isfile(fname):
            # compile any mod files
            os.chdir(os.path.join("tests", dr))
            # remove old compiled files
            try:
                shutil.rmtree(platform.machine())
            except OSError:
                pass
            os.system("nrnivmodl")
            os.chdir(dname)
            with open(fname) as f:
                files += [os.path.join(dr, s) for s in f.read().splitlines()]
    sys.exit(test(files, correct_data))
