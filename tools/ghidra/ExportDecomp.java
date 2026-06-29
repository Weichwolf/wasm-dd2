// DD2 pipeline stage 1: export the FULL named decompile from the analyzed Ghidra project.
// Names/types live in the project DB, so this output is human-readable by construction.
// Output dir is passed via the GHIDRA_DD2_OUT property (default re_out/). Writes:
//   <out>/dd2_decomp.c    - every decompiled function, "/* ===== name @ addr ===== */" delimited
//   <out>/functions.txt   - entrypoint \t numAddresses \t name  (the function index)
// Run via tools/decompile.sh (analyzeHeadless -postScript). Uses the inherited `monitor`
// field (ConsoleTaskMonitor import is not portable across Ghidra versions).
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import java.io.*;

public class ExportDecomp extends GhidraScript {
    public void run() throws Exception {
        String OUT = System.getProperty("GHIDRA_DD2_OUT",
            "/home/cosmo/Git/wasm-dd2/re_out");
        new File(OUT).mkdirs();
        FunctionManager fm = currentProgram.getFunctionManager();

        PrintWriter idx = new PrintWriter(new FileWriter(OUT + "/functions.txt"));
        for (Function f : fm.getFunctions(true))
            idx.println(f.getEntryPoint() + "\t" + f.getBody().getNumAddresses() + "\t" + f.getName());
        idx.close();

        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        PrintWriter out = new PrintWriter(new BufferedWriter(new FileWriter(OUT + "/dd2_decomp.c")));
        int n = 0, tot = 0;
        for (Function f : fm.getFunctions(true)) {
            tot++;
            try {
                DecompileResults res = di.decompileFunction(f, 60, monitor);
                if (res != null && res.decompileCompleted()) {
                    out.println("\n/* ===== " + f.getName() + " @ " + f.getEntryPoint() + " ===== */");
                    out.print(res.getDecompiledFunction().getC());
                    n++;
                }
            } catch (Exception e) {
                println("FAIL " + f.getName() + " @ " + f.getEntryPoint() + ": " + e.getMessage());
            }
            if ((n % 200) == 0) out.flush();
        }
        out.close();
        println("EXPORT_DONE exported " + n + "/" + tot + " functions -> " + OUT);
    }
}
