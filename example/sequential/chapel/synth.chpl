/*
 *  Synthetic Performance Application
 *
 */
use hclient;

// Configuration variables for program:
config var n = 240;

var numThreads = numThreadsPerLocale;
if (numThreads == 0) {
    numThreads = here.numCores;
}

/* For illustration purposes, the performance here is defined by following
 * simple definition:
 *   perf = (p1-9)*(p1-9) + (p2-8)*(p2-8) + 
 *          (p3-7)*(p3-7) + (p4-6)*(p4-6) + 
 *          (p4-5)*(p4-5) + (p5-4)*(p5-4) +
 *          (p6-3)*(p6-3) + 200
 * All parameters are in [1-100] range
 * 
 */
proc application(p1, p2, p3, p4, p5, p6: int) 
{
    var perf = 
        (p1-45)*(p1-9) + (p2-65)*(p2-8) + 
        (p3-85)*(p3-7) - (p4-75)*(p4-6) +
        (p5-55)*(p5-4) -
        (p6-45)*(p6-3) - 200;

    return perf;
}

proc critical_loop(i : int)
{
    var hdesc : opaque;
    var p1, p2, p3, p4, p5, p6, hresult : int;
    var perf : real(64);

    hdesc = harmony_init("SearchT", HARMONY_IO_POLL);

    if (harmony_register_int(hdesc, "param_1", p1, 1, 100, 1) < 0 ||
        harmony_register_int(hdesc, "param_2", p2, 1, 100, 1) < 0 ||
        harmony_register_int(hdesc, "param_3", p3, 1, 100, 1) < 0 ||
        harmony_register_int(hdesc, "param_4", p4, 1, 100, 1) < 0 ||
        harmony_register_int(hdesc, "param_5", p5, 1, 100, 1) < 0 ||
        harmony_register_int(hdesc, "param_6", p6, 1, 100, 1) < 0)
    {
        writeln(i, ": Failed to register variable");
        return;
    }

    if (harmony_connect(hdesc, "localhost", 0) < 0)
    {
        writeln(i, ": Failed to connect to server");
        return;
    }

    for j in 1..n by numThreads align i do
    {
        hresult = harmony_fetch(hdesc);

        perf = application(p1, p2, p3, p4, p5, p6);

        if (hresult == 0)
        {
            writeln(i, "[", j, "]: Not ready!");
        }
        else {
            writeln(i, "[", j, "]: (", p1, ", ", p2, ", ", p3, ", ",
                    p4, ", ", p5, ", ", p6, ") = ", perf);
            harmony_report(hdesc, perf);
        }
    }

    harmony_disconnect(hdesc);
    return;
}

proc main()
{
    coforall i in 1..numThreads do
        critical_loop(i);

    return 0;
}
