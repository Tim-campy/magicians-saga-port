// Focused disassembly/decompilation of caller-selected ELF VAs, not full analysis.
// Input TSV: hex ELF VA, tab, exclusive end VA from FDE, tab, analyst label.
// Requires ELF with minimum load VA 0. FDE coverage is an explicit analysis bound.
// @category MagiciansSaga
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.io.PrintWriter;

public class InspectSelectedMethods extends GhidraScript {
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) throw new IllegalArgumentException("selection-tsv output-directory");
        Path out = Paths.get(args[1]);
        Files.createDirectories(out);
        DecompInterface decompiler = new DecompInterface();
        try (PrintWriter report = new PrintWriter(Files.newBufferedWriter(out.resolve("selected.tsv"), StandardCharsets.UTF_8))) {
            report.println("# sha256=" + currentProgram.getExecutableSHA256());
            report.println("# image_base=" + currentProgram.getImageBase());
            report.println("elf_va\tghidra_address\tlabel\tstatus");
            if (!decompiler.openProgram(currentProgram)) throw new IllegalStateException("Decompiler unavailable");
            for (String line : Files.readAllLines(Paths.get(args[0]), StandardCharsets.UTF_8)) {
                if (line.isBlank() || line.startsWith("#")) continue;
                String[] fields = line.split("\t");
                if (fields.length != 3 || !fields[2].matches("[A-Za-z0-9_]+")) throw new IllegalArgumentException("Bad selection");
                long va = Long.parseUnsignedLong(fields[0].replaceFirst("^0x", ""), 16);
                long endVa = Long.parseUnsignedLong(fields[1].replaceFirst("^0x", ""), 16);
                if (endVa <= va || endVa-va > 1048576) throw new IllegalArgumentException("Invalid FDE bound");
                Address address = currentProgram.getImageBase().add(va);
                Address end = currentProgram.getImageBase().add(endVa-1);
                AddressSet body = new AddressSet(address, end);
                var block = currentProgram.getMemory().getBlock(address);
                if (block == null || !block.isExecute()) throw new IllegalArgumentException("Non-executable address " + address);
                if (!disassemble(address)) throw new IllegalStateException("Disassembly failed at " + address);
                Function f = getFunctionAt(address);
                if (f == null) f = createFunction(address, fields[2]);
                if (f == null) throw new IllegalStateException("No function at " + address);
                f.setBody(body);
                f.setName(fields[2], SourceType.USER_DEFINED);
                // Give outgoing direct callees/tail targets distinct function entries so
                // the decompiler does not absorb them into this FDE-bounded method.
                for (var ins : currentProgram.getListing().getInstructions(body, true)) {
                    if (!ins.getFlowType().isCall() && !ins.getFlowType().isJump()) continue;
                    for (Address target : ins.getFlows()) {
                        if (body.contains(target) || getFunctionAt(target) != null) continue;
                        var targetBlock = currentProgram.getMemory().getBlock(target);
                        if (targetBlock == null || !targetBlock.isExecute() || getFunctionContaining(target) != null) continue;
                        currentProgram.getFunctionManager().createFunction("callee_" + target, target,
                            new AddressSet(target, target), SourceType.ANALYSIS);
                    }
                }
                decompiler.flushCache();
                DecompileResults result = decompiler.decompileFunction(f, 60, monitor);
                String status = result.decompileCompleted() ? "complete" : result.getErrorMessage();
                report.println(fields[0] + "\t" + address + "\t" + fields[2] + "\t" + status.replace('\n', ' '));
                if (result.decompileCompleted()) Files.writeString(out.resolve(fields[2] + ".c"), result.getDecompiledFunction().getC(), StandardCharsets.UTF_8);
                StringBuilder asm = new StringBuilder();
                for (var ins : currentProgram.getListing().getInstructions(f.getBody(), true)) asm.append(ins.getAddress()).append("\t").append(ins).append("\n");
                Files.writeString(out.resolve(fields[2] + ".asm"), asm.toString(), StandardCharsets.UTF_8);
                println(fields[2] + ": " + status);
            }
        } finally { decompiler.dispose(); }
    }
}
