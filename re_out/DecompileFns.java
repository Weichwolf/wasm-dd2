import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.*;

public class DecompileFns extends GhidraScript {
    public void run() throws Exception {
        long[] addrs = {0x417ea0L,0x41861cL,0x418ed0L,0x41bc0cL,0x41bc68L,0x41c3e4L,
                        0x41c440L,0x41ccf8L,0x41cdd0L,0x41d834L,0x41d918L,0x41e418L};
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        PrintWriter out = new PrintWriter(new FileWriter("/home/cosmo/Git/wasm-dd2/re_out/handlers.c"));
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
