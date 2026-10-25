// Focused smali export using the installed JADX API; no target execution.
import jadx.api.JadxArgs;
import jadx.api.JadxDecompiler;
import jadx.api.JavaClass;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.HexFormat;

public class ExportSmali {
    public static void main(String[] args) throws Exception {
        if (args.length < 3) throw new IllegalArgumentException("apk output-directory class [class...]");
        Path apk = Path.of(args[0]);
        Path out = Path.of(args[1]);
        Files.createDirectories(out);
        JadxArgs config = new JadxArgs();
        config.setInputFile(apk.toFile());
        config.setThreadsCount(1);
        config.setSkipResources(true);
        config.setSkipSources(true);
        try (JadxDecompiler jadx = new JadxDecompiler(config)) {
            jadx.load();
            for (int i=2; i<args.length; i++) {
                if (!args[i].matches("[A-Za-z0-9_.$]+")) throw new IllegalArgumentException("Invalid class selector");
                JavaClass cls = jadx.searchJavaClassByOrigFullName(args[i]);
                if (cls == null) throw new IllegalArgumentException("Missing exact class: " + args[i]);
                String smali = cls.getSmali();
                if (smali == null || smali.isBlank()) throw new IllegalStateException("Missing smali: " + args[i]);
                Files.writeString(out.resolve(args[i] + ".smali"), smali, StandardCharsets.UTF_8);
                System.out.println("Exported " + args[i]);
            }
        }
        String digest = HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(Files.readAllBytes(apk)));
        Files.writeString(out.resolve("apk.sha256"), digest + "\n", StandardCharsets.UTF_8);
    }
}
