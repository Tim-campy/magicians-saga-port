# Registro e despacho do sistema de animação ECS

Data: 2026-10-25. Investigação estática complementar aos relatórios28–31. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Nenhum APK executado, scheduler implementado ou frequência medida.

## Método e limites

Oito FDEs foram exportados com `ExportBoundedInstructions.java`, seleção `build/research/ecs-order.tsv`, saída `build/research/ecs-order/`. Todos têm cobertura sintática integral, registrada em `coverage.tsv`. Foi reutilizado `animation-producers/M37431_Init.asm` do ciclo de produtores. Os endereços abaixo são VAs ELF; exports Ghidra somam `0x100000`.

Uma varredura independente dos words alinhados nos segmentos executáveis encontrou nove BL/B imediatos para os alvos selecionados: constructor/Init/Update de AnimationSystem, Add/AddSystems/Init/Update de SystemManager e AddSystems/Update de World. Resultado local `ecs-order/direct-references.json`. Ausência de chamada direta a Init/Update de AnimationSystem não implica ausência de execução: o despacho por interface foi resolvido abaixo. A varredura não prova alcance nem cobre indireções.

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java build/research/ecs-order.tsv build/research/ecs-order -max-cpu 1
```

## Criação e inclusão

`Systems.CreateSystems`37991 (`0x116883c`) aloca um array de **32** referências, com argumento `0x20` em `0x11689f8`. No trecho da animação:

- Aloca a classe do GOT ELF `0x2731b38`, chama AnimationSystem..ctor37441 em `0x1169054` e chama BaseSystem.set_enabled37187 com argumento true em `0x1169068`.
- Grava a referência em array+`0xb8` em `0x1169094`: elemento **19**, considerando dados do array em+0x20 e referências de8bytes.
- O elemento18 é construído pela chamada ao construtor de **ModifyAnimatinSystem** (grafia original), store `0x1169040`; o elemento20 é **CellCullingSystem**, store `0x11690e8`. RenderSystem aparece no elemento21, store `0x116913c`. Estes são posições no array geral, não índices finais de uma lista filtrada de Update.

Identidade da alocação AnimationSystem corroborada por GOT→slot metadata `0x2731b38→0x27c4c98`, valor `0x20004685`, índice de tipo nativo9026→metadata4450. CellCullingSystem: `0x2731b58→0x27c55c0`, `0x20004d77`, índice9915→metadata4457. Isso evita inferir o tipo alocado apenas pelo último construtor chamado: por exemplo, o elemento27 chama o construtor BasePauseSystem, mas sua alocação GOT `0x2731be8→0x27ca080`, `0x200080cd`, corresponde a **SubMenuSystem**, metadata4495. `factory-order.json` registra chamadas a construtores observadas, explicitamente não tipos de alocação para todas as entradas.

`AppData.InitWorld`34262 (`0x1101c48`) lê o World em seu bloco estático+0x18. Se World.byte+0x38 está zero, chama CreateSystems (`0x1101cb0` ou `0x1101ccc`) e passa o array para World.AddSystems (`0x1101ce0`). Depois cria o mapa e encaminha para World.Init. O significado completo/escritores do flag+0x38 não foi recuperado; não atribuir-lhe automaticamente nome de save/carregamento.

World.AddSystems36995 encaminha a SystemManager.AddSystems37218 em `0x11357c8`. Este percorre o array **em índice crescente**, chamando Add37217 para cada entrada (`0x113c180–0x113c1a0`).

## Listas por interface e ordem inicial

SystemManager.Add (`0x113bbd4`) insere a referência na coleção geral this+0x20 e consulta interfaces. Para IInitSystem, acrescenta à lista this+0x28 (`0x113bd40–0x113bdbc`); para IUpdateSystem, acrescenta à lista this+0x30 (`0x113bdc0–0x113be44`). O código escreve na posição de tamanho atual e incrementa esse tamanho, ou chama o caminho de expansão; não há ordenação no corpo observado. Outras listas de desenho/pausa também são preenchidas, mas seus contratos completos não foram reconstruídos aqui.

As identidades usadas no teste e despacho foram conferidas no ELF/metadata:

| GOT ELF → slot metadata | Codificado | Tipo nativo | Metadata |
| --- | --- | ---: | --- |
| `0x2731118→0x27c7270` | `0x200061ed` | 12534 | 4400 IInitSystem |
| `0x2731120→0x27c76f8` | `0x2000645f` | 12847 | 4403 IUpdateSystem |

Portanto a inclusão inicial preserva a ordem relativa dos membros compatíveis do array de fábrica. Não declarar que AnimationSystem é o vigésimo Update nem que a ordem permanece imutável: nem todos os32 sistemas necessariamente implementam a interface e enable/disable, remoção e desserialização podem alterar as listas. Seus efeitos ainda precisam ser seguidos.

## Init e Update realmente chegam à AnimationSystem

SystemManager.Init37220 percorre this+0x28 e chama o método0 da interface IInitSystem (`0x113c6d4–0x113c72c`). O metadata de AnimationSystem4450 tem vtableStart58921 e interface offset13 para tipo12534; entrada `0x6001246f` resolve a **37431 AnimationSystem.Init**.

SystemManager.Update37221 (`0x113c7d0`) percorre this+0x30 em índice crescente. Para cada sistema:

1. Chama o método0 de IUpdateSystem em `0x113c8ec`, passando o argumento inteiro recebido pelo manager em w1.
2. Se o bit0 do retorno está desligado, pula Update (`0x113c8f0`).
3. Caso contrário chama o método1 da mesma interface, com o mesmo argumento, em `0x113c960`.

AnimationSystem tem interface offset14 para tipo12847; entradas `0x60012471/0x60012473` resolvem a **37432 NeedsUpdate /37433 Update**. Registro local em `interface-dispatch.json`. NeedsUpdate é o corpo de8bytes que retorna true, já exportado em `animation-transitions/`; ele não filtra iterações adicionais pelo argumento. Este trecho do manager não testa diretamente BaseSystem.byte+0x18: não acrescentar esse filtro sem examinar o mecanismo de enable/disable que mantém listas.

AnimationSystem.Init cria filtro com inteiros **2 e10**, chama Filter.And e World.GetSubSet, armazena o subconjunto em this+0x20 (`0x1141ab0–0x1141b04`). Esses IDs correspondem aos componentes Seb e Animation conferidos nos produtores do relatório31. Também aloca array de24 ResourceManagers e o preenche consultando AppData.GetResourceManager para índices0..23 (`0x1141b08–0x1141b90`). Essa é a origem das coleções consumidas pelo Update descrito no relatório29.

## Ligação com o evento lógico do formulário

O relatório28 ligou evento1 ao slot19 do formulário. Para a classe **QuestForm**, metadata4280/vtableStart57376, slot19 contém `0x6001181d`→método **35854 QuestForm.Update**, VA `0x11daee4`. Assim há uma cadeia estática específica:

```text
DoMethod(evento1) → QuestForm.Update → World.Update
→ SystemManager.Update → IUpdateSystem.NeedsUpdate/Update
→ AnimationSystem.Update (para essa entrada da lista)
```

QuestForm.Update usa World em this+0x120 e chama World.Update em `0x11db32c`; este encaminha para SystemManager.Update em `0x1135844`, preservando argumento inteiro. Há ainda uma multiplicação de passagens **dentro** de QuestForm.Update, independente do loop de FormManagerBase:

- Em `0x11db010–0x11db048`, se Config.static.byte+0x10 está ligado e Canvas.CheckKeyState(0x10) tem bit0 ligado, escolhe limite3; caso contrário limite1. Semântica física da máscara e ativação runtime não são inferidas.
- w20 começa zero em `0x11db2f0`; cada chamada World.Update recebe esse índice e ele é incrementado até o limite (`0x11db324–0x11db338`).
- Para índices após zero, consulta MyFormBase.Active35796; retorno falso pula aquela chamada. Se ativo, GetTopForm precisa ser o próprio formulário; mudança sai do trecho (`0x11db2f4–0x11db318`). A primeira passagem não faz esses dois testes locais.

O corpo anterior contém outras condições e alterações de estado. Esses sites não provam que todas as três chamadas ocorrem numa partida. Comprovam, porém, que equiparar um evento1 a exatamente um AnimationSystem.Update também seria uma simplificação indevida.

## Limites e próximo passo

Cobertura validada: oito FDEs novos integralmente exportados, identidade de interfaces via relocação/metadata, entradas virtuais específicas e chamadas diretas confrontadas com assembly. Sem testes dinâmicos, sem execução de gameplay reconstruído. Não foi modificado código C++ neste trabalho.

Próximos alvos estáticos: manutenção das listas em enable/disable/deserialize; origem e escritores do modo que permite passagens extras; métodos de criação do mundo e transições que escolhem QuestForm. Os produtores dos componentes e a integração das transições podem usar esta cadeia, preservando separação entre contador do framework, evento do formulário, passagem ECS e frame da animação.

Continuação em [33-ecs-lifecycle.md](33-ecs-lifecycle.md): EnableSystem/DisableSystem não removem nem reinserem diretamente itens; seus delegates podem ser chamados mesmo sem mudança de estado. Para AnimationSystem, hooks básicos são vazios e o despacho examinado não adiciona filtro enabled. A hipótese genérica de manutenção das listas por esses métodos não deve virar regra do port.
