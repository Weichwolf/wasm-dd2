// Set variadic (printf-style) signatures on the sprintf-family wrappers so Ghidra recovers the pushed
// format args at call sites (the fresh dd2h analysis dropped them; the dd2.exe program had them typed).
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.data.*;
import ghidra.program.model.symbol.SourceType;
public class SetVarargs extends GhidraScript {
    public void run() throws Exception {
        long[] addrs = { 0x4568ee };  // FUN_004568ee = the sprintf wrapper (dd2.exe FUN_0045672e shifted)
        DataTypeManager dtm = currentProgram.getDataTypeManager();
        for (long va : addrs) {
            Function f = getFunctionAt(toAddr(va));
            if (f == null) { println("no fn at " + Long.toHexString(va)); continue; }
            f.setVarArgs(true);
            f.setReturnType(new IntegerDataType(), SourceType.USER_DEFINED);
            // params: char* buf, char* fmt
            f.replaceParameters(Function.FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS, true,
                SourceType.USER_DEFINED,
                new ParameterImpl("buf", new PointerDataType(new CharDataType()), currentProgram),
                new ParameterImpl("fmt", new PointerDataType(new CharDataType()), currentProgram));
            println("set variadic: " + f.getName() + " @ " + Long.toHexString(va));
        }
    }
}
