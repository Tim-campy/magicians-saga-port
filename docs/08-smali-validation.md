# Conferência do bootstrap e loop em smali

Data: 2026-10-25. Foram lidas as instruções DEX apresentadas em smali pelo JADX 0.7.1 para as classes exatas `com.unity3d.player.UnityPlayer`, `T0` e `NativeLoader`. Mesma cópia de APK/hash dos relatórios anteriores; nenhum código executado. O wrapper REA usado não oferece seleção explícita de smali quando a decompilação Java tem sucesso, portanto foi usada a API `JavaClass.getSmali()` do JAR instalado, com assinaturas verificadas por `javap`.

## Resolução das dúvidas

- `UnityPlayer.loadNative(String)`: o intervalo `try_start_13..try_end_16` contém `System.load` do caminho completo. `UnsatisfiedLinkError` direciona a `catch_19`, onde ocorre `System.loadLibrary("main")`. `SecurityException` vai a `catch_17`. Se o fallback falha com `UnsatisfiedLinkError`, vai a `catch_3a`; ambos os caminhos de erro retornam a mensagem de `logLoadLibMainError`. Sucesso alcança `goto_1e` e `NativeLoader.load`. Isso esclarece a estrutura confusa de try/catch recuperada em Java.
- `T0.handleMessage`: o ramo `S0.e` seta a flag `c` e, quando `d == 3`, salta para `goto_e1`. O ramo `S0.g` seta `d=3` e salta ao mesmo bloco quando `c` é verdadeiro. O bloco comum chama a ponte para `nativeFocusChanged(true)` e seta `d=1`. O aviso PHI/duplicação do Java corresponde a esse bloco compartilhado, sem comprovar erro do código original.
- O ramo `S0.f` só encaminha `nativeFocusChanged(false)` se `d == 1`, e depois seta `d=2`.
- O bloco `goto_10a` só enfileira outra mensagem `0x8dd` (2269), objeto `S0.h`, quando `b` é verdadeiro e `e <= 0`; incrementa `e` após `sendToTarget`. Os retornos antecipados no ramo de renderização não atravessam esse bloco. Não há duração de frame estabelecida por isso.
- `NativeLoader.load` e `unload` têm modificador explícito `native`; a ambiguidade native/abstract do resultado de método JADX anterior está resolvida. `UnityPlayer.initJni` também pode ser conferido no arquivo completo antes de tipar sua implementação nativa.

Confiança `confirmed` para os ramos e modificadores acima sob a representação DEX do JADX. Timing, execução concorrente real, primeira cena e implementação dos métodos nativos continuam fora desta evidência. A semântica de flags obfuscadas não foi renomeada por suposição.

## Reprodução

```powershell
java -Xmx768m -cp .tools/jadx/jadx-headless-mcp-0.7.1-all.jar tools/ExportSmali.java "Magician's+Saga_1.4.0_APKPure.apk" build/research/smali com.unity3d.player.T0 com.unity3d.player.UnityPlayer com.unity3d.player.NativeLoader
```

Saídas proprietárias locais em `build/research/smali/*.smali`, ignoradas pelo Git; identidade da entrada em `apk.sha256`. O exportador usa uma thread e não salva projeto Java ou recursos.

Limitação de inventário: a carga direta do JADX registrou 19.137 classes internas no log e aviso de duplicação de `DebugProbesKt` (entrada `.bin` e classes.dex); o bridge REA havia reportado 12.499 classes. A diferença de critérios/configuração de contagem não foi reconciliada. Nenhuma conclusão sobre total global depende desse novo número: a conferência foi limitada às três classes selecionadas por nome original exato. Não substituir silenciosamente a contagem anterior.
