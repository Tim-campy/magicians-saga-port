// ARM64 FDE-bounded linear disassembly export; no decompiler or flow expansion.
// TSV: ELF VA hex, exclusive end ELF VA hex, label [A-Za-z0-9_]+.
// ELF minimum load VA must be zero; output applies current Ghidra image base.
// API verified against installed Ghidra Base-src.zip DisassembleCommand.java.
// Syntactic instruction coverage does not prove reachability or function semantics.
// @category MagiciansSaga
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.HexFormat;
import java.io.PrintWriter;

public class ExportBoundedInstructions extends GhidraScript {
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) throw new IllegalArgumentException("selection-tsv output-directory");
        if (!currentProgram.getLanguageID().toString().startsWith("AARCH64:LE:64:"))
            throw new IllegalArgumentException("Only ARM64 little-endian is supported");
        Path out = Paths.get(args[1]);
        Files.createDirectories(out);
        try (PrintWriter report = new PrintWriter(Files.newBufferedWriter(out.resolve("coverage.tsv"), StandardCharsets.UTF_8))) {
            report.println("# elf_sha256=" + currentProgram.getExecutableSHA256());
            report.println("# image_base=" + currentProgram.getImageBase());
            report.println("# language=" + currentProgram.getLanguageID());
            report.println("# coverage is syntactic; no reachability or decompiler claims");
            report.println("elf_va\tend_exclusive\tlabel\tbytes\tinstructions\tcovered_bytes\tmissing_words\tcommand_ok\tstatus\tbytes_sha256");
            report.flush();
            for (String line : Files.readAllLines(Paths.get(args[0]), StandardCharsets.UTF_8)) {
                monitor.checkCancelled();
                if (line.isBlank() || line.startsWith("#")) continue;
                String[] f = line.split("\t");
                if (f.length != 3 || !f[2].matches("[A-Za-z0-9_]+"))
                    throw new IllegalArgumentException("Bad selection row");
                long va = Long.parseUnsignedLong(f[0].replaceFirst("^0x", ""),16);
                long end = Long.parseUnsignedLong(f[1].replaceFirst("^0x", ""),16);
                if (va < 0 || end <= va || end-va > 1048576 || (va&3)!=0 || (end&3)!=0)
                    throw new IllegalArgumentException("Invalid aligned FDE bounds");
                Address start = currentProgram.getImageBase().add(va);
                Address last = currentProgram.getImageBase().add(end-1);
                AddressSet bounds = new AddressSet(start,last);
                AddressSet starts = new AddressSet();
                int size = (int)(end-va);
                for (int offset=0;offset<size;offset+=4) {
                    Address address=start.add(offset);
                    var block=currentProgram.getMemory().getBlock(address);
                    if (block==null || !block.isExecute() || !block.isInitialized())
                        throw new IllegalArgumentException("Unmapped/non-executable FDE word");
                    starts.add(address);
                }
                byte[] bytes=new byte[size];
                if (currentProgram.getMemory().getBytes(start,bytes)!=size)
                    throw new IllegalStateException("Incomplete bytes read");
                String digest=HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(bytes));
                // Attempt each aligned word; never follow a call or branch outside bounds.
                var command=new DisassembleCommand(starts,bounds,false);
                boolean ok=command.applyTo(currentProgram,monitor);
                int count=0,missing=0;
                StringBuilder assembly=new StringBuilder();
                StringBuilder gaps=new StringBuilder("elf_va\tghidra_address\traw_hex\n");
                for (int offset=0;offset<size;offset+=4) {
                    Address address=start.add(offset);
                    var ins=currentProgram.getListing().getInstructionAt(address);
                    if (ins!=null && ins.getLength()==4 && bounds.contains(ins.getMaxAddress())) {
                        assembly.append(address).append('\t').append(ins).append('\n');
                        count++;
                    } else {
                        gaps.append(Long.toHexString(va+offset)).append('\t').append(address).append('\t')
                            .append(HexFormat.of().formatHex(bytes,offset,offset+4)).append('\n');
                        missing++;
                    }
                }
                Files.writeString(out.resolve(f[2]+".asm"),assembly,StandardCharsets.UTF_8);
                Files.writeString(out.resolve(f[2]+".gaps.tsv"),gaps,StandardCharsets.UTF_8);
                String status=missing==0 ? "full_syntactic_coverage" : "partial_coverage";
                report.println(f[0]+"\t"+f[1]+"\t"+f[2]+"\t"+size+"\t"+count+"\t"+(count*4)+"\t"+missing+"\t"+ok+"\t"+status+"\t"+digest);
                report.flush();
                println(f[2]+": "+status+", "+(count*4)+"/"+size+" bytes");
            }
        }
    }
}
