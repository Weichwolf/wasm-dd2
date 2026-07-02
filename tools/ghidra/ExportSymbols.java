import ghidra.app.script.GhidraScript;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import java.io.*;

public class ExportSymbols extends GhidraScript {
    public void run() throws Exception {
        PrintWriter out = new PrintWriter(new FileWriter("/home/cosmo/Git/wasm-dd2/re_out/data_symbols.txt"));
        SymbolIterator it = currentProgram.getSymbolTable().getDefinedSymbols();
        int n = 0;
        while (it.hasNext()) {
            Symbol s = it.next();
            if (s.getSymbolType() == SymbolType.LABEL) {
                Address a = s.getAddress();
                if (a != null && a.isMemoryAddress()) {
                    Data d = getDataAt(a);
                    out.println(s.getName() + "\t" + a + "\t" + (d != null ? d.getLength() : 0));
                    n++;
                }
            }
        }
        out.close();
        println("exported " + n + " data symbols");
    }
}
