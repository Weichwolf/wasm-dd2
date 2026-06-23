import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.*;

public class DecompileFns extends GhidraScript {
    public void run() throws Exception {
        long[] addrs = {0x4191acL,0x419b94L,0x41a66cL,0x41b2d8L};
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        PrintWriter out = new PrintWriter(new FileWriter("/home/cosmo/Git/wasm-dd2/re_out/subdivhandlers.c"));
        for (long a : addrs) {
            Address addr = toAddr(a);
            Function f = getFunctionAt(addr);
            if (f == null) {
                disassemble(addr);
                try { f = createFunction(addr, "FUN_00"+Long.toHexString(a)); } catch (Exception e) {}
            }
            if (f == null) { out.println("// FUN_00"+Long.toHexString(a)+" - no function"); continue; }
            DecompileResults res = di.decompileFunction(f, 90, monitor);
            if (res != null && res.decompileCompleted()) {
                out.println("/* ===== FUN_00"+Long.toHexString(a)+" ===== */");
                out.println(res.getDecompiledFunction().getC());
                println("ok FUN_00"+Long.toHexString(a));
            } else {
                out.println("// FUN_00"+Long.toHexString(a)+" - FAILED: "+(res==null?"null":res.getErrorMessage()));
                println("FAIL FUN_00"+Long.toHexString(a));
            }
        }
        out.close();
        println("DECOMPILE_DONE");
    }
}
