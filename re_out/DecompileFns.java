import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.*;

public class DecompileFns extends GhidraScript {
    public void run() throws Exception {
        long[] addrs = {0x417efcL,0x4180c8L,0x41828cL,0x418458L,0x418678L,0x41888cL,0x418aa4L,0x418cb8L,0x418fe0L,0x4196a8L,0x4199b0L,0x41a0acL,0x41a40cL,0x41acf4L,0x41b054L,0x41b97cL,0x41bcc4L,0x41be90L,0x41c054L,0x41c220L,0x41c49cL,0x41c6b0L,0x41c8c8L,0x41cae0L,0x41ce8cL,0x41d058L,0x41d360L,0x41d5e8L,0x41d9e0L,0x41dbf4L,0x41df54L,0x41e184L};
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        PrintWriter out = new PrintWriter(new FileWriter("/home/cosmo/Git/wasm-dd2/re_out/facehandlers.c"));
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
