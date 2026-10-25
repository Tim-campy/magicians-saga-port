// Read a caller-selected JNINativeMethod table from the loaded Ghidra image.
// @category MagiciansSaga
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;

public class InspectJniTable extends GhidraScript {
    private String cstring(long value) throws Exception {
        Address address = toAddr(value);
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < 512; i++) {
            int b = currentProgram.getMemory().getByte(address.add(i)) & 255;
            if (b == 0) return result.toString();
            if (b < 32 || b > 126) throw new IllegalArgumentException("Non-ASCII JNI string");
            result.append((char)b);
        }
        throw new IllegalArgumentException("Unterminated JNI string");
    }
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 3 || currentProgram.getDefaultPointerSize() != 8)
            throw new IllegalArgumentException("Expected 64-bit program and address count output-file");
        Address base = toAddr(args[0]);
        int count = Integer.parseInt(args[1]);
        if (count < 1 || count > 64) throw new IllegalArgumentException("Invalid count");
        StringBuilder report = new StringBuilder("table_address\tname\tsignature\tfunction\n");
        for (int i=0; i<count; i++) {
            Address entry = base.add(i * 24L);
            long name = currentProgram.getMemory().getLong(entry);
            long signature = currentProgram.getMemory().getLong(entry.add(8));
            long function = currentProgram.getMemory().getLong(entry.add(16));
            report.append(entry).append("\t").append(cstring(name)).append("\t")
                .append(cstring(signature)).append("\t").append(toAddr(function)).append("\n");
        }
        Files.writeString(Paths.get(args[2]), report.toString(), StandardCharsets.UTF_8);
        println(report.toString());
    }
}
