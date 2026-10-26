# Bootstrap nativo ARM64

Data: 2026-10-25. Análise estática com Ghidra 12.1.4 headless, linguagem `AARCH64:LE:64:v8A`, heap 768 MiB e uma CPU. A rota direta funcionou no Windows; o limite ARM ELF do bridge REA não é um limite do Ghidra instalado.

## Artefato e cobertura

Membro `lib/arm64-v8a/libmain.so` do APK fornecido, 6.728 bytes; SHA-256 `9348cd29bcdec5b677c7124b111a2ebb285bfd6550282619c8fd178f2982eb5d`. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`. Extração seletiva em `build/research/native`, sem executar o artefato.

Ghidra importou somente essa biblioteca, concluiu autoanálise e exportou nove funções não-thunk, todas com pseudocódigo disponível. Bibliotecas externas Android não foram carregadas: 11 símbolos externos não resolvidos. O registro de debug aponta `libmain.dbg.so`, indisponível; não há informação DWARF recuperada. Pseudocódigo não é código original nem evidência de execução.

## Mapa com endereços

Ghidra escolheu base `0x00100000`. A coluna VA ELF subtrai essa base; não é endereço ASLR em execução. Offsets de arquivo foram conferidos pelos segmentos PT_LOAD.

| Função / tabela | Endereço Ghidra | VA ELF | Offset de arquivo |
|---|---|---|---|
| `JNI_OnLoad` exportada | `0x00104a70` | `0x4a70` | `0x0a70` |
| implementação de `NativeLoader.load` | `0x00104b00` | `0x4b00` | `0x0b00` |
| implementação de `NativeLoader.unload` | `0x00104bdc` | `0x4bdc` | `0x0bdc` |
| helper de carregamento dinâmico | `0x00104cd8` | `0x4cd8` | `0x0cd8` |
| tabela JNINativeMethod, duas entradas | `0x0010d138` | `0xd138` | `0x1138` |

`JNI_OnLoad` obtém JNIEnv, busca `com/unity3d/player/NativeLoader` e registra duas entradas. Leitura direta da memória relocada pelo Ghidra confirmou:

| Método | Protótipo DEX/JNI | Função Ghidra |
|---|---|---|
| `load` | `(Ljava/lang/String;)Z` | `0x00104b00` |
| `unload` | `()Z` | `0x00104bdc` |

Protótipos concordam com `research/local/unity-NativeLoader-class.json` do JADX. Isso resolve a implementação de `NativeLoader.load`, que o JADX havia reportado sem corpo.

Os slots de ponteiros foram confrontados com o `include/jni.h` do JDK Microsoft 21 instalado: AttachCurrentThread (`0x20` na tabela VM), FindClass (`0x30`), FatalError (`0x90`), GetStringUTFLength/Chars/Release (`0x540/0x548/0x550`), RegisterNatives (`0x6b8`) e GetJavaVM (`0x6d8`) na tabela JNIEnv de 64 bits. O assembly confirma a chamada RegisterNatives com contagem 2 em `0x00104ab8..0x00104ac4` e retorno `0x10006` no caminho de sucesso.

## Comportamento observado estaticamente

**confirmed:** a função em `0x00104b00` copia a string de diretório recebida do Java, chama o helper para `libunity.so` e depois para `libil2cpp.so`, libera a cópia e retorna se o handle de Unity está preenchido. Esse booleano não testa diretamente os dois handles; não substituir por uma condição presumida de que ambos foram carregados.

**confirmed:** o helper em `0x00104cd8` retorna se já existe handle. Caso contrário, obtém a VM, forma `<diretório>/<biblioteca>`, chama `dlopen` com flag numérica 1 e tenta apenas o nome da biblioteca se a primeira tentativa falhar. Procura `JNI_OnLoad` com `dlsym`; se presente, chama-o. O assembly em `0x00104d8c..0x00104d98` faz comparação signed com `0x10007`. Não reescrever essa condição como uma validação mais restritiva presumida. Falhas selecionadas passam mensagens a FatalError; isso não é uma simples exceção Java recuperável.

**confirmed:** `unload` trata os handles de Unity e IL2CPP, busca `JNI_OnUnload` opcionalmente, chama-o e fecha a biblioteca via `dlclose` nos caminhos recuperados. Detalhes de falhas e lifecycle precisam de teste em runtime antes de reprodução.

**unknown:** implementação de `UnityPlayer.initJni`, registro de `nativeRender/nativeInjectEvent`, funções internas de `libunity`, entrada IL2CPP e primeira cena. Não foi descompilado `libil2cpp.so` nesta etapa. ARM32 não foi comparado.

## Evidências locais e reprodução

- `build/research/native/extraction-manifest.json`: hash da entrada e do membro extraído.
- `build/research/libmain-arm64/functions.tsv`: funções, nomes recuperados, base e estado da decompilação.
- `build/research/libmain-arm64/jni-table.tsv`: tabela lida da memória relocada.
- `build/research/libmain-arm64/<endereço>.asm` e `.c`: disassembly e pseudocódigo, locais e ignorados pelo Git.
- `build/research/ghidra-analysis.log` e `ghidra-followup.log`: importação, cobertura e mensagens do Ghidra.

Primeira importação, em projeto novo (não repetir `-import` sobre projeto já existente):

```powershell
python tools/extract_research_members.py "Magician's+Saga_1.4.0_APKPure.apk" --output build/research/native --member lib/arm64-v8a/libmain.so
New-Item -ItemType Directory -Force build/research/ghidra
$env:GHIDRA_HEADLESS_MAXMEM = '768M'
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_bootstrap -import build/research/native/lib/arm64-v8a/libmain.so -scriptPath tools/ghidra -postScript InspectBootstrap.java build/research/libmain-arm64 -max-cpu 1 -analysisTimeoutPerFile 120
```

Reutilizar projeto salvo para a tabela:

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_bootstrap -process libmain.so -noanalysis -scriptPath tools/ghidra -postScript InspectJniTable.java 0010d138 2 build/research/libmain-arm64/jni-table.tsv -max-cpu 1
```

Os scripts são ferramentas de pesquisa, não o extrator de assets do futuro port. Não há mudanças de gameplay ou build PC nessa etapa.
