# Callbacks nativos: chamadas e limites

Data: 2026-10-25. Análise estática ARM64, sem execução do jogo. Identidades de APK, ELF e metadata em `07-il2cpp-code-map.md`.

## Método

Os tokens IL2CPP selecionam as entradas; `.eh_frame` fornece intervalos FDE independentes da próxima entrada conhecida. Ghidra usa base `0x100000`; os endereços abaixo são VAs ELF, sem essa base.

| Método main.Main | Índice | Início | Fim exclusivo FDE | Bytes |
|---|---:|---|---|---:|
| OnCreate | 34537 | 0x117c78c | 0x117cdc8 | 1596 |
| OnUpdate | 34538 | 0x117cdc8 | 0x117da00 | 3128 |
| OnDraw | 34539 | 0x117db20 | 0x117dbe0 | 192 |

FDE descreve cobertura de unwind, não prova de alcance de todas as instruções nem de limites lógicos absolutos. `tools/inspect_arm64_calls.py` lê palavras ARM64 alinhadas nesse intervalo, reconhece BL/B/BLR/BR e associa destinos diretos ao mapa IL2CPP. Mantém chamadas indiretas sem alvo e B externo como candidato a transferência de cauda.

```powershell
python tools/inspect_unwind_ranges.py build/research/il2cpp/lib/arm64-v8a/libil2cpp.so --output build/research/il2cpp-unwind.json
python tools/inspect_arm64_calls.py build/research/il2cpp/lib/arm64-v8a/libil2cpp.so research/local/il2cpp-code-map.json build/research/il2cpp-unwind.json --method-index 34537 --method-index 34538 --method-index 34539 --output research/local/main-callback-transfers.json
```

Inventário: 137.201 FDEs; 265 transferências selecionadas nos três intervalos. Os resultados proprietários permanecem locais.

## Observações confirmadas no código

| Origem | Destinos diretos selecionados | Implicação limitada |
|---|---|---|
| OnCreate | IApplication.OnCreate `0x1b86d54`; native.AppPlugin.Init `0x117adcc`; IApplication.SetViewport `0x1b87c34` | Inicialização do framework e plugin; configuração de viewport |
| OnCreate | Canvas.SetBackground `0x1b55bc8`; Canvas.SetDisplayCurrent `0x1b4cf6c`; FormManagerBase.ChangeCurrentForm `0x1bdee14` | Preparação de apresentação e formulário; formulário inicial ainda não identificado |
| OnCreate | RecordStore.ReadRecord `0x1c98344`; JarInflater..ctor `0x1bfba70`; StringUtil.LoadStringArray `0x1c19cdc`; Language.SetTextTable `0x1b1bea8` | Leitura de registros e tabelas; formato e conteúdo ainda precisam de investigação |
| OnUpdate | FormManagerBase.Execute `0x1be00f0`; main.Main.SetJingle `0x117da00`; Canvas.CheckKeyState `0x1b53eac` | Caminhos de atualização, áudio e entrada presentes |
| OnUpdate | QualitySettings.set_vSyncCount `0x23a3300`; FormManagerBase.GetTargetFps `0x1be17f8`; Application.set_targetFrameRate `0x23953e0` | Controle explícito de apresentação/taxa; não estabelece timestep fixo |
| OnUpdate | SaveRecorder.SaveSystem `0x1bac1e0`; AppData.AddErrorLog `0x110f368`; KairoBase.IsIllegalPackage `0x1c79f8c` | Persistência, diagnóstico e verificação de pacote presentes; condições e efeitos não recuperados |
| OnDraw | FormManager.GetInstance `0x11d06f8`, seguido de BLR | Despacho indireto; alvo virtual ainda não resolvido |

Em `OnCreate`, chamada em `0x117c918`, as instruções precedentes carregam `w1=0xf0`, `w2=0x118`, `x0=this`, `x3=0`. Assim, há argumentos inteiros **240 e 280** para `SetViewport`. Isso não prova resolução física, escala final ou layout completo.

Em `OnDraw`, `0x117db28` chama GetInstance; `0x117db30` lê a classe do objeto, `0x117db34` lê o par em `classe+0x178`, e `0x117db38` executa `blr x9`. Não atribuir nome ao slot sem resolver o layout/vtable.

Atualização: a corroboração de layout de classe em `13-animation-formats.md` fornece origem de vtable `0x138`, stride 16. Portanto `+0x178` corresponde ao slot 4. No metadata, `form.FormManager` (tipo 4274) possui vtableStart 57317; slot 4 contém `0x600099bb`, que resolve para método **19677, FormManagerBase.OnDraw**. Esse é o destino declarado para essa classe; o tipo concreto do objeto em runtime ainda não foi capturado. Para `main.Main` (tipo 4166, vtableStart 55788), slots 5/11/12 resolvem precisamente para os métodos 34537/34538/34539, corroborando os overrides OnCreate/OnUpdate/OnDraw.

## Validação e falhas preservadas

Três tentativas Ghidra (sem limites, com corpos limitados e com 67 funções semeadas a partir de FDE) não produziram pseudocódigo confiável de OnUpdate: permanecem avisos e trechos de callees fora do intervalo. Não usar esse pseudocódigo para implementar regras. Os artefatos ficam em `build/research/il2cpp-callbacks*`.

A comparação entre varredura de bytes e assembly exportado encontrou **75/77 BL** em OnCreate, **157/165 BL** em OnUpdate e **13/13 BL** em OnDraw. Todos os BL exportados coincidem em endereço e destino; Ghidra deixou lacunas em caudas/caminhos de exceção. Os dez BL adicionais dos bytes não foram validados por um segundo disassembler e não fundamentam afirmações de comportamento neste relatório. A divergência é explícita; execução bem-sucedida do exportador não equivale a recuperação integral.

Atualização: o exportador posterior `ExportBoundedInstructions.java`, sem decompiler e sem seguir fluxo fora dos intervalos, cobriu todos os words dos três FDEs em `build/research/lifecycle-asm`. Nova comparação bateu **77/77, 165/165 e 13/13 BL**, incluindo os dez anteriormente ausentes. Isso resolve a divergência de inventário das chamadas, mas não recupera pseudocódigo nem prova alcance runtime. Os resultados antigos foram preservados como histórico.

## Segunda rodada: funções pequenas

Exportações em `build/research/small-methods`, seleção em `build/research/small-methods.tsv`, pelo script `InspectSelectedMethods.java` no projeto local `wizard_il2cpp_fde`.

- `IApplication.SetViewport`, índice 18264, FDE de 8 bytes em `0x1b87c34`: somente `stp w1,w2,[x0,#0x5c]` e retorno. Portanto os valores 240/280 são armazenados nos offsets de instância `0x5c/0x60`; o método não configura diretamente uma superfície de renderização.
- `FormManagerBase.GetTargetFps`, índice 19733, FDE de 60 bytes em `0x1be17f8`: chama `IsFormChanging` (`0x1be4858`); se bit 0 do retorno estiver ativo, retorna **120**. Caso contrário consulta `GetTopForm` (`0x1bdcd34`), usa inteiro em `form+0x4c` se houver formulário e valor diferente de -1; nos demais casos retorna inteiro em `manager+0xb0`. Nomes desses campos, valores iniciais e taxa efetivamente alcançada permanecem desconhecidos. Assembly e pseudocódigo dessa pequena função concordam.
- `main.Main.SetJingle`, índice 34543, FDE de 288 bytes em `0x117da00`: chama `SoundPlayer.GetInstance` (`0x1bb8f60`) e termina em transferência para `SoundPlayer.SetVolume` (`0x1bbb21c`). Um caminho passa volume zero; outro passa resultado de `SecureInt.op_Implicit` (`0x1c06880`) multiplicado por 51. Não inferir unidade, faixa de preferência nem motivo desses caminhos sem resolver campos e chamadores. O pseudocódigo absorveu a função de destino; a evidência aqui é o assembly delimitado, não esse pseudocódigo.

Próximos passos: resolver o slot virtual de desenho, valores iniciais de timing e formatos de recursos; obter comparação em dispositivo Android antes de afirmar fidelidade de timing, áudio ou UI.
