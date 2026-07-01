// Create function objects at addresses that have names but no function body (so ExportDecomp emits them).
// Reads addrs from /tmp/create_fns.txt ("0xADDR name" per line). For the dd2h re-base: the fns were
// hand-created in the dd2.exe program but Ghidra auto-analysis didn't recreate them in dd2h.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.app.cmd.function.CreateFunctionCmd;
import java.io.*;
import java.util.*;
public class CreateMissingFns extends GhidraScript {
    public void run() throws Exception {
        BufferedReader r = new BufferedReader(new FileReader("/tmp/create_fns.txt"));
        String line; int made=0, had=0, fail=0;
        while ((line = r.readLine()) != null) {
            line = line.trim(); if (line.isEmpty()) continue;
            String[] p = line.split("\\s+", 2);
            long va = Long.decode(p[0]);
            Address a = toAddr(va);
            Function f = getFunctionAt(a);
            if (f != null) { had++; continue; }
            CreateFunctionCmd cmd = new CreateFunctionCmd(a);
            if (cmd.applyTo(currentProgram, monitor)) { made++; if (p.length>1) getFunctionAt(a).setName(p[1], ghidra.program.model.symbol.SourceType.USER_DEFINED); }
            else fail++;
        }
        r.close();
        println("CreateMissingFns: created " + made + ", already-had " + had + ", failed " + fail);
    }
}
