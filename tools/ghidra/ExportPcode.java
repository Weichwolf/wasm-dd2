// DD2 pipeline: prototype P-code lifter input. Exports the RAW (low) P-code for one function,
// with varnodes formatted as register names / constants / uniques / RAM. This is the
// register-explicit IR that resolves the C decompiler's `unaff_EBX` problem: registers are
// just named state, so a P-code->WASM/C emitter is bit-faithful with no hand-reconstruction.
// Target address via -Dgridra... property GHIDRA_PCODE_ADDR (default 0x413f45 = a GTE subfn).
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.pcode.PcodeOp;
import ghidra.program.model.pcode.Varnode;
import ghidra.program.model.lang.Register;

public class ExportPcode extends GhidraScript {
    String vn(Varnode v) {
        if (v == null) return "-";
        if (v.isConstant()) return "#0x" + Long.toHexString(v.getOffset());
        if (v.isRegister()) {
            Register r = currentProgram.getRegister(v.getAddress(), v.getSize());
            return (r != null ? r.getName() : ("reg+0x"+Long.toHexString(v.getOffset()))) + ":" + v.getSize();
        }
        if (v.isUnique()) return "u" + Long.toHexString(v.getOffset()) + ":" + v.getSize();
        return v.getAddress().getAddressSpace().getName() + "[0x" + Long.toHexString(v.getOffset()) + "]:" + v.getSize();
    }
    // machine-parseable varnode: TYPE:value:size  (R=register C=const U=unique M=ram-const)
    String vnm(Varnode v) {
        if (v == null) return "-";
        if (v.isConstant()) return "C:" + Long.toHexString(v.getOffset()) + ":" + v.getSize();
        if (v.isRegister()) {
            Register r = currentProgram.getRegister(v.getAddress(), v.getSize());
            return "R:" + (r != null ? r.getName() : ("r"+Long.toHexString(v.getOffset()))) + ":" + v.getSize();
        }
        if (v.isUnique()) return "U:" + Long.toHexString(v.getOffset()) + ":" + v.getSize();
        return "M:" + Long.toHexString(v.getOffset()) + ":" + v.getSize();
    }
    public void run() throws Exception {
        long addr = Long.decode(System.getProperty("GHIDRA_PCODE_ADDR", "0x413f45"));
        Address a = toAddr(addr);
        Function f = getFunctionContaining(a);
        java.io.PrintWriter pw = new java.io.PrintWriter(new java.io.FileWriter(
            System.getProperty("GHIDRA_PCODE_OUT", "/tmp/pcode_fn.txt")));
        pw.println("# FUNC " + (f!=null?f.getName():"?") + " @ " + a);
        for (Instruction insn : currentProgram.getListing().getInstructions(f.getBody(), true)) {
            pw.println("I\t" + insn.getAddress() + "\t" + insn.toString());
            int seq = 0;
            for (PcodeOp op : insn.getPcode()) {
                StringBuilder in = new StringBuilder();
                Varnode[] ins = op.getInputs();
                for (int i = 0; i < ins.length; i++) { if (i>0) in.append(","); in.append(vnm(ins[i])); }
                pw.println("P\t" + insn.getAddress() + "\t" + (seq++) + "\t" +
                    vnm(op.getOutput()) + "\t" + op.getMnemonic() + "\t" + in);
            }
        }
        pw.close();
        println("FUNC " + (f != null ? f.getName() : "?") + " @ " + a + "  body=" +
                (f != null ? f.getBody().getNumAddresses() : 0) + " bytes");
        Listing lst = currentProgram.getListing();
        AddressSetView body = (f != null) ? f.getBody() : null;
        InstructionIterator it = lst.getInstructions(body, true);
        int ni = 0, nop = 0;
        while (it.hasNext()) {
            Instruction insn = it.next();
            println(insn.getAddress() + ":  " + insn.toString());
            for (PcodeOp op : insn.getPcode()) {
                nop++;
                Varnode out = op.getOutput();
                StringBuilder sb = new StringBuilder("      ");
                if (out != null) sb.append(vn(out)).append(" = ");
                sb.append(op.getMnemonic());
                Varnode[] in = op.getInputs();
                for (int i = 0; i < in.length; i++) sb.append(i == 0 ? " " : ", ").append(vn(in[i]));
                println(sb.toString());
            }
            ni++;
        }
        println("PCODE_DONE instructions=" + ni + " pcode_ops=" + nop);
    }
}
