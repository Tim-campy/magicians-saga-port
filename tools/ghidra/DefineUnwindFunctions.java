// Define function analysis boundaries from verified ELF unwind FDE records.
// TSV: ELF start VA, exclusive ELF end VA, label. Minimum ELF load VA must be 0.
// @category MagiciansSaga
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;

public class DefineUnwindFunctions extends GhidraScript {
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Expected FDE selection TSV");
        int count = 0;
        for (String line : Files.readAllLines(Paths.get(args[0]), StandardCharsets.UTF_8)) {
            if (line.isBlank() || line.startsWith("#")) continue;
            String[] row = line.split("\t");
            if (row.length != 3 || !row[2].matches("[A-Za-z0-9_]+")) throw new IllegalArgumentException("Bad FDE row");
            long start = Long.parseUnsignedLong(row[0],16), end = Long.parseUnsignedLong(row[1],16);
            if (start >= end) throw new IllegalArgumentException("Bad FDE range");
            var address = currentProgram.getImageBase().add(start);
            var last = currentProgram.getImageBase().add(end-1);
            var block = currentProgram.getMemory().getBlock(address);
            if (block == null || !block.isExecute() || !block.contains(last)) throw new IllegalArgumentException("FDE outside executable block");
            var f = getFunctionAt(address);
            if (f == null) f = currentProgram.getFunctionManager().createFunction(row[2], address, new AddressSet(address,last), SourceType.ANALYSIS);
            else f.setBody(new AddressSet(address,last));
            f.setName(row[2], SourceType.USER_DEFINED);
            count++;
        }
        println("Defined " + count + " FDE-backed function boundaries");
    }
}
