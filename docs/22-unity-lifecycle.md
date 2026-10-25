# Ciclo Unity: entradas do framework

Data: 2026-10-25. Observações estáticas, sem execução do APK. Este mapa complementa o loop Android descrito em `04-android-startup.md`; não substitui uma captura de fluxo.

`tools/inspect_arm64_calls.py` varreu seis intervalos FDE de `kairo.unity.ui.IApplication`, gerando `research/local/unity-lifecycle-calls.json`. São 1.037 transferências candidatas; existência de instrução não comprova alcance de um caminho, ordem dinâmica nem alvo de chamadas indiretas.

| Entrada | Metadata index | VA ELF | Bytes FDE | Destinos diretos selecionados |
|---|---:|---|---:|---|
| Awake | 18195 | 0x1b7e2a4 | 9536 | Application.persistentDataPath/productName/platform/temporaryCachePath/dataPath, CopyDirectory, Screen e Canvas |
| Start | 18197 | 0x1b80c70 | 1444 | Time.set_fixedDeltaTime, SetResolutionScaleRatio, Screen.sleepTimeout, OnApplicationFocus |
| FixedUpdate | 18198 | 0x1b815c4 | 4 | Nenhuma transferência |
| Update | 18199 | 0x1b815c8 | 4476 | Canvas.Update/input, SoundPlayer.UpdateJingle, ExecuteUIMethods, ExecuteUpdateMethods |
| LateUpdate | 18200 | 0x1b83714 | 548 | TerminateCheck, UpdateResolution, ExecuteUIMethods |
| OnGUI | 18202 | 0x1b83954 | 6612 | Canvas.BeginPaint/EndPaint, OnInputUpdate, UpdateGUI, DrawGUI |

O único word de FixedUpdate é `0xd65f03c0` (`RET X30`). Portanto esse callback do framework não contém atualização de gameplay. Isso não exclui FixedUpdate em outros componentes; não assumir que a configuração TimeManager de 0,02 s define a simulação do jogo.

Call sites de referência: `Start+0x108`, VA `0x1b80d78`, chama `Time.set_fixedDeltaTime` (`0x23c24f8`); argumento ainda não resolvido. `Update`, VA `0x1b821b0`, chama `SoundPlayer.UpdateJingle` (`0x1bbbcfc`), e `0x1b821c0` chama `ExecuteUpdateMethods` (`0x1b8303c`). OnGUI contém BeginPaint em `0x1b83d08` e EndPaint em `0x1b83e74`; condições de execução não recuperadas nessa varredura.

## Ligação dos hooks confirmada no assembly

Nova exportação sem decompiler, por `ExportBoundedInstructions.java`, seleção em `build/research/lifecycle-asm.tsv`: nove intervalos (seis entradas e três callbacks Main) com cobertura sintática integral, hashes e lacunas em `build/research/lifecycle-asm/coverage.tsv`. Essa cobertura não prova que todas as instruções são alcançadas.

O metadata de main.Main declara overrides OnCreate/OnUpdate/OnDraw nos slots 5/11/12, conforme `09-native-callbacks.md`. A origem de vtable corroborada é `classe+0x138`, stride 16:

- **Awake → OnCreate:** `x26` recebe this em `0x1b7e2d0` e permanece o receptor até `0x1b7f138..0x1b7f144`, que lê o par `classe+0x188` (slot 5), passa this e executa BLR. Para a classe main.Main, o slot resolve ao método 34537.
- **Update → OnUpdate:** `x19` recebe this em `0x1b815f4`; `0x1b821c4..0x1b821d0` lê o par `classe+0x1e8` (slot 11) e chama com esse receptor. Resolve ao método 34538 na classe Main.
- **OnGUI → OnDraw:** this é preservado de x26 na pilha em `0x1b83f2c`; `0x1b841fc..0x1b84208` recupera esse ponteiro e chama o par `classe+0x1f8` (slot 12), correspondente ao método 34539. Há caminhos que saltam essa chamada; condições completas ainda não reconstruídas.

O segundo uso de slot 5 encontrado em Awake, `0x1b7f250`, usa outro objeto (`x24`), e não foi confundido com o hook da aplicação.

### Configuração de timestep

Imediatamente antes do BL de Start para `Time.set_fixedDeltaTime` em `0x1b80d78`, a instrução em `0x1b80d70` carrega **float 1,0** em s0 (`0x3f800000`). Assim, o código possui um caminho que substitui a configuração serializada 0,02 por 1,0 segundo. Somado ao FixedUpdate vazio, isso reforça que não se deve implementar a simulação como 50 Hz por leitura do arquivo de configuração. O momento de execução, outras alterações e efeitos observados ainda precisam de runtime.

Ainda faltam condições de pausa, eventos GUI e frequência efetiva. Não chamar o mapa de grafo dinâmico, nem implementar um loop fixo de 50/60/120 Hz com base apenas nesses nomes.
