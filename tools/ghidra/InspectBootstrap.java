// Export a bounded native bootstrap inventory and pseudocode for local research.
// @category MagiciansSaga
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.io.PrintWriter;

public class InspectBootstrap extends GhidraScript {
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Expected output directory");
        Path out = Paths.get(args[0]);
        Files.createDirectories(out);
        DecompInterface decompiler = new DecompInterface();
        try (PrintWriter report = new PrintWriter(Files.newBufferedWriter(out.resolve("functions.tsv"), StandardCharsets.UTF_8))) {
            if (!decompiler.openProgram(currentProgram)) throw new IllegalStateException("Cannot open decompiler");
            report.println("# program=" + currentProgram.getName());
            report.println("# sha256=" + currentProgram.getExecutableSHA256());
            report.println("# language=" + currentProgram.getLanguageID());
            report.println("# image_base=" + currentProgram.getImageBase());
            report.println("address\tname\texternal\tthunk\tdecompile_status");
            int count = 0;
            for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
                monitor.checkCancelled();
                String status = "skipped";
                if (!f.isExternal() && !f.isThunk() && count < 100) {
                    count++;
                    DecompileResults result = decompiler.decompileFunction(f, 30, monitor);
                    status = result.decompileCompleted() ? "complete" : "failed: " + result.getErrorMessage().replace('\n', ' ');
                    if (result.decompileCompleted()) {
                        Files.writeString(out.resolve(f.getEntryPoint().toString() + ".c"),
                            "/* " + f.getName() + " @ " + f.getEntryPoint() + " */\n" + result.getDecompiledFunction().getC(), StandardCharsets.UTF_8);
                    }
                    StringBuilder assembly = new StringBuilder();
                    for (var instruction : currentProgram.getListing().getInstructions(f.getBody(), true)) {
                        assembly.append(instruction.getAddress()).append("\t").append(instruction).append("\n");
                    }
                    Files.writeString(out.resolve(f.getEntryPoint().toString() + ".asm"), assembly.toString(), StandardCharsets.UTF_8);
                }
                report.println(f.getEntryPoint() + "\t" + f.getName() + "\t" + f.isExternal() + "\t" + f.isThunk() + "\t" + status);
            }
            println("Bootstrap export complete: " + count + " functions attempted; " + out);
        } finally { decompiler.dispose(); }
    }
}
