# Condições estáticas de atualização, desenho e pausa

Data: 2026-10-25. Continuação de `22-unity-lifecycle.md`, sem executar o APK. Há avanço estático útil: os callbacks Unity não encaminham incondicionalmente cada chamada a `main.Main`.

## Evidência e limites

ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`, APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`. Todos os endereços deste relatório são **VAs ELF**; subtrair `0x100000` dos endereços nos exports Ghidra.

Fontes locais: `build/research/lifecycle-asm/M18199_Update.asm`, `M18202_OnGUI.asm`, mapa de métodos `research/local/il2cpp-code-map.json` e quatro novos intervalos em `build/research/frame-gates/`. Estes últimos têm cobertura sintática integral, registrada em `coverage.tsv`: TotalFrames 148 bytes, OnApplicationPause 1.752 bytes, OnApplicationFocus 564 bytes, TerminateCheck 760 bytes. Não foi usado pseudocódigo nem assumido alcance dinâmico.

Reprodução da exportação adicional, usando o projeto importado:

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java build/research/frame-gates.tsv build/research/frame-gates -max-cpu 1
```

Notação: `I.static+N` significa o bloco estático da classe obtida pelo slot GOT ELF `0x272f528`, através de `classe+0xb8`. `C.static+N` usa o slot `0x272f5c0`. Os offsets são evidência; nomes de campos não foram atribuídos apenas pela ordem da metadata. Chamadas ao verificador/inicializador de classes e saídas excepcionais foram omitidas das descrições, não declaradas inexistentes.

## Update → OnUpdate

O único call site do hook slot 11 continua sendo `0x1b821d0` (receiver this). O caminho normal contém os seguintes filtros comprovados:

| Condição / bloco ELF | Consequência observada |
| --- | --- |
| `I.static+0x40 != null`, `0x1b81744–0x1b817a0` | Não chega ao hook. Inicializa timestamp em this+0xc8; antes de 3.001 ms retorna; a partir disso encaminha para `TerminateCheck(this,true)`. |
| this+0x140, `0x1b81df8–0x1b81e28` | Subtrai 1 em registrador com semântica de 32 bits. Resultado negativo segue caminho normal; zero chama Application.Quit; resultado positivo é armazenado em +0x140 e pula o hook. Portanto não modelar como contador irrestritamente decrementado em todo frame. |
| `TerminateCheck(this,false)`, `0x1b81e80–0x1b81e8c` | Retorno com bit 0 ativo pula atualização principal. |
| `C.static+0x30 != 0 && this+0x35 != 0`, `0x1b81f58–0x1b81f68` | Pula atualização principal. +0x35 recebe diretamente o booleano de OnApplicationPause, conforme abaixo. O seletor C+0x30 ainda não foi nomeado; não declarar a regra universal em todas as plataformas. |
| `I.static+0x48 != null`, `0x1b8214c–0x1b82154` | Pula SoundPlayer.UpdateJingle, ExecuteUIMethods, ExecuteUpdateMethods e o hook; segue para o delegate Updated em this+0xe8, se presente. O objeto em +0x48 é usado por SplashScreen.Draw em OnGUI. |

Antes do último filtro, o caminho incrementa this+0x58 em `0x1b82080–0x1b82088`. O getter **IApplication.get_TotalFrames**, método 18187, lê exatamente esse campo do singleton em `0x1b7de38–0x1b7de44`. Logo TotalFrames pode avançar enquanto o hook Main.OnUpdate é pulado pela presença do splash; não equiparar esse contador a ticks efetivos de gameplay.

A ordem comprovada no trecho que efetivamente chega ao hook é: SoundPlayer.UpdateJingle (`0x1b821b0`), ExecuteUIMethods (`0x1b821b8`), ExecuteUpdateMethods (`0x1b821c0`), Main.OnUpdate (`0x1b821d0`), delegate Updated opcional (`0x1b82210`). O byte this+0xf9 é ligado antes desse trecho e desligado no caminho normal e em caminhos de tratamento de exceção; não é, por si só, um relógio.

O timestamp this+0x148 é escrito antes em `0x1b81d38–0x1b81d44` por JSystem.CurrentTimeMillis. O campo this+0x110 recebe a duração medida em `0x1b82258–0x1b82268`. Não há prova aqui de duração fixa por tick.

## OnGUI → OnDraw

Há uma saída alternativa inteira para `I.static+0x40 != null`: desenho de mensagem usando Canvas/Graphics, seguido de retorno; não passa pelo hook principal. O caminho sem esse objeto passa pelos filtros abaixo:

1. `TerminateCheck(this,false)` em `0x1b83ea8`; retorno verdadeiro sai.
2. `UpdateResolution` em `0x1b83eb4`; o valor de this+0x138 menos 1 é testado em `0x1b83eb8–0x1b83ec8`. Resultado não negativo é armazenado e sai. Resultado negativo prossegue sem armazená-lo.
3. Byte this+0x128 diferente de zero sai em `0x1b83f18–0x1b83f1c`.
4. Liga this+0xf8. Se não há splash em I.static+0x48, executa ExecuteUIMethods (`0x1b83f50`).
5. Se this+0x1f8 é não zero, chama UpdateGUI (`0x1b83f60`); retorno com bit 0 **desligado** salta diretamente para preparação do desenho. Essa exceção impede afirmar que todo desenho exige o mesmo Event.type.
6. No caminho restante, lê Event.current.type. Tipos numéricos 0, 3 ou 1 acionam Canvas.OnInputUpdate (`0x1b83fa0–0x1b83ff8`). Depois, somente **tipo 7 e this+0x20 == 0** prosseguem; as outras combinações desligam this+0xf8 e retornam (`0x1b83ffc–0x1b84024`). Os números são conservados aqui sem depender de nomes externos do enum.

Após preparação de Graphics e SEB, há outras duas alternativas ao hook:

- Se this+0x80 é verdadeiro e this+0x88 não nulo, obtém Texture2D desse Image e chama GUI.DrawTexture centralizado (`0x1b840cc–0x1b84178`); pula Main.OnDraw.
- Caso contrário, se I.static+0x48 não nulo, chama **SplashScreen.Draw** (`0x1b841c0`). Se o byte do splash +0x20 está ligado, limpa I.static+0x48, mas ainda não chama Main.OnDraw nessa passagem. Sem splash, finalmente chama slot 12 em `0x1b84208`.

`Seb.ThisFrameRenderAnimation` é chamado em `0x1b84090` somente quando I.static+0x1c está ligado; `Seb.ResetThisFrameRender` em `0x1b840b0` não depende desse byte nesse caminho. Update liga I.static+0x1c em `0x1b820a8`. A eventual limpeza desse byte e a política completa de animação exigem seguir os métodos SEB; não duplicar avanço de animação a cada evento GUI por suposição.

## Espera explícita após desenho

O final de OnGUI contém uma espera condicional, não um timestep universal. `C.static+0x63` habilita o trecho (`0x1b84af8–0x1b84b14`). Obtém o objeto retornado pelo slot 15 do aplicativo e chama **FormManagerBase.GetTargetFps**, método 19733, em `0x1b84b34`.

No caminho de FPS positivo finito, calcula `budget = trunc(float(1000) / float(fps))` em inteiro de 64 bits (`0x1b84b38–0x1b84b5c`). Há tratamento explícito de +infinito no código; não expandir a fórmula simplificada a zero/valores negativos/overflow. Se `CurrentTimeMillis() - this+0x148 >= budget`, retorna. Caso contrário, faz ciclos de **JThread.Sleep(1)** (`0x1b84bdc`) e ExecuteUIMethods (`0x1b84be4`).

O ciclo tem duas saídas de tempo: o orçamento desde +0x148 e metade do orçamento desde um timestamp local. Quando ExecuteUIMethods retorna valor >=1, reinicia esse timestamp local (`0x1b84be8–0x1b84bf0`), mas não o limite total desde +0x148. Isso é diferente de dormir sempre `1000/fps` depois de cada atualização. Ainda falta resolver o seletor C+0x63, os valores efetivos de FPS e a execução desse ramo no Android.

## Pausa, foco e encerramento

**OnApplicationPause** (18205, `0x1b8570c`) retorna cedo se I.static+0x40 não nulo; caso contrário grava `argumento & 1` em this+0x35 (`0x1b857bc–0x1b857c0`). Com pausa ativa, chama NotifyManager.DoNotify, KairoPlugin.SetNotificationBackground(true), percorre formas de trás para frente enviando código 4 e pode desativar FormManagerBase. Na retomada, chama DoCancel, SetNotificationBackground(false), envia código 3 às formas e contém caminhos para KairoAccount.Resume, StartAsyncConnectThread, ClockTime.UpdateServerTime, GooglePlayLicenseCheck e AudioSettings.Reset. Esses destinos são símbolos e chamadas estáticas, não prova de que fazem rede, que a licença exige conectividade ou de que todos os ramos ocorrem neste aparelho.

**OnApplicationFocus** (18208, `0x1b81390`) grava o argumento em this+0x34 no caminho `0x1b81470–0x1b81474`. Há filtros dependentes de C.static+0x30 e this+0x178 antes disso. Perda de foco no caminho aceito desativa FormManagerBase, chama hook virtual e SoundPlayer.Suspend (`0x1b814dc`). Em um ramo, encaminha o inverso do foco a OnApplicationPause (`0x1b81428–0x1b81434`). Portanto foco e pausa não são eventos intercambiáveis sem condições.

**TerminateCheck** (18210, `0x1b82744`) usa I.static+0x60 como habilitador, retorno de GetFormsNum e argumento booleano. Com argumento falso, splash presente e Config.IsXbox verdadeiro, retorna falso antecipadamente (`0x1b827f0–0x1b82834`). Na sequência, ausência de manager ou contagem zero pode habilitar encerramento; argumento verdadeiro também satisfaz essa condição. Exige contador this+0x140 <=0. Chama IApplication.Terminate (`0x1b828f8`), e depois compara Canvas.static+0x50 com this+0x58 (`0x1b829f0–0x1b82a00`): igualdade grava **3** em this+0x140, diferença chama Application.Quit imediatamente. Retorna verdadeiro nesse caminho. Assim, o contador visto em Update tem produtor e uso no encerramento, não é uma frequência de simulação.

## Próximos passos exclusivamente estáticos

- Resolver nomes/valores iniciais de C.static+0x30/+0x63 e I.static+0x1c/+0x60 por getters, setters e inicializador estático, ou pela tabela nativa de offsets validada; não inferi-los da ordem dos campos.
- Seguir FormManagerBase.GetTargetFps e seus setters/callers para obter política de FPS por formulário.
- Seguir Seb.ThisFrameRenderAnimation/ResetThisFrameRender e o consumidor de I.static+0x1c para fechar avanço entre Update e eventos de desenho.
- Resolver os hooks de pausa/retomada no vtable Main e mensagens 3/4 no FormManager antes de reconstruir efeitos de jogo.

Os testes dinâmicos continuam necessários para confirmar sequência e frequência reais, mas **não são o único avanço disponível**. Nenhuma regra de gameplay foi implementada com base em condições ainda incompletas.
