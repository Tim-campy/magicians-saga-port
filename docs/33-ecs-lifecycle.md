# Habilitação, pausa e reconstrução dos sistemas ECS

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`, ELF ARM64 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Continuação do relatório32, sem execução do APK.

## Evidência

Doze FDEs em `build/research/ecs-lifecycle/`, seleção `ecs-lifecycle.tsv`, foram exportados sem decompilador com cobertura sintática integral. Foram reutilizados SystemManager.Add/Update e World.Update de `ecs-order/`. Endereços abaixo são VAs ELF; exports Ghidra somam `0x100000`.

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java build/research/ecs-lifecycle.tsv build/research/ecs-lifecycle -max-cpu 1
```

## Habilitação não é remoção da lista

BaseSystem.set_enabled37187 (`0x113b48c`) compara instance.byte+0x18 com argumento&1. Se iguais retorna. Se diferentes, escreve o byte **antes** do hook: slot4 OnEnabled para true, slot5 OnDisabled para false (`0x113b4a0–0x113b4b8`). O setter não altera listas do manager nem notifica seus delegates diretamente.

SystemManager.EnableSystem37227 (`0x113cc18`) percorre o array de tipos recebido e consulta GetSystem37231. Para cada resultado não nulo:

1. Se ainda não habilitado, escreve byte+0x18=1 e chama slot4 (`0x113cc68–0x113cc88`).
2. Independentemente de ter mudado o byte, se existir delegate manager+0x10, invoca-o com o sistema (`0x113cc8c–0x113cca4`).

DisableSystem37229 (`0x113c568`) é simétrico: só chama slot5 quando o byte estava ligado, mas o delegate manager+0x18 é invocado mesmo quando já estava desligado (`0x113c5b0–0x113c5e8`). Ambos preservam ordem do array de pedidos; duplicatas não são descartadas por esses corpos. Tipos não encontrados são pulados.

Os nomes dos delegates são corroborados pelos métodos metadata add_OnSystemEnabled/Disabled. Uma varredura BL/B imediato para os quatro add/remove desses eventos não encontrou callers diretos; isso não prova ausência de assinatura por outro caminho, inlining ou reflexão. Não inventar uma rotina de remoção/reinserção da lista de Update como efeito obrigatório dessas notificações.

## Caso específico da animação

A vtable metadata de AnimationSystem4450 começa em58921; slots4/5 contêm `0x6001228b/0x6001228d`, resolvendo a BaseSystem.OnEnabled37189/OnDisabled37190. Os dois corpos exportados são apenas RET (`0x113b520/0x113b524`). Seu NeedsUpdate37432 retorna true, conforme relatório32.

SystemManager.Update percorre a lista+0x30, consulta NeedsUpdate e chama Update se true; não testa diretamente o byte enabled do sistema nem manager.isPause no corpo examinado. World.Update36999 (`0x1135804`) encaminha ao manager sem teste local de pausa/enable (`0x1135834–0x1135844`).

Assim, **nesse encadeamento, alterar somente enabled de AnimationSystem não é suficiente para eliminar seu despacho**. Isso não declara que animações continuam em toda pausa real: o formulário pode parar de atualizar o World, callbacks externos podem intervir, e a composição das listas pode mudar por outros caminhos. O port não deve adicionar um filtro genérico `if (!enabled) skip` por hábito; seria comportamento novo em relação ao fluxo verificado.

## Pausa: propriedade versus operação

get_isPause37215 lê manager.byte+0x70; set_isPause37216 apenas grava argumento&1 (`0x113bbc0–0x113bbd0`). O setter **não** despacha callbacks.

PauseSystems37232 (`0x113b03c`) sempre grava o mesmo byte em `0x113b0ec–0x113b0f0`, sem saída antecipada por estado repetido. Percorre a coleção+0x48, resolve método0 da interface pelo slot GOT `0x2731050` e passa o booleano (`0x113b14c–0x113b1a4`). A identificação nominal dessa interface ainda não foi fechada nesta rodada; não confundir com IUpdateSystem.

Quando argumento=true, depois percorre a coleção geral+0x20 e chama slot6 de cada sistema (`0x113b1b8–0x113b204`). Na vtable BaseSystem e AnimationSystem, slot6 é OnPause37191. Quando argumento=false, essa segunda fase não ocorre. O conteúdo concreto de cada hook pode variar com o tipo; não substituir ambas as fases por simples alteração de enabled.

## Remoção total e ordenação

RemoveAllSystems37219 (`0x113c1bc`) enumera primeiro a coleção geral. Para cada sistema obtém seu tipo, cria array unitário e chama DisableSystem (`0x113c2e8–0x113c340`). Portanto hooks/notificações acontecem **antes** da limpeza das coleções; não limpar o armazenamento antes desses callbacks.

Após essa fase, zera tamanhos/incrementa versões e limpa referências usadas das coleções nos offsets+0x20,+0x28,+0x30,+0x38,+0x40,+0x48,+0x58,+0x60. A estrutura em+0x50 recebe chamada genérica separada (`0x113c45c–0x113c468`), cujo nome não foi atribuído por aparência. Isso é diferente do DisableSystem individual, que não contém esses blocos de limpeza.

Caller direto observado: World.Destroy36991 em `0x1135754`. Não foi inferida uma equivalência entre Destroy e pausa ou troca de formulário.

## Serialize/Deserialize e dependência de ordem

SystemManager.Serialize37235 (`0x113ce34`) percorre a lista geral+0x20 em índice crescente. Para cada sistema chama slots **11,7,9**, respectivamente OnBeforeSerialize, Serialize e SerializeExtra, corroborados na vtable BaseSystem. Call sites `0x113ceb0`, `0x113cec4`, `0x113ced8`. Não há escrita própria de quantidade/ID do sistema nesse wrapper; os métodos chamados podem escrever formatos próprios.

Deserialize37236 (`0x113cefc`) chama Systems.CreateSystems em `0x113cf64` e AddSystems em `0x113cf70`, depois enumera a coleção geral+0x20 e chama slots **8,10,12**: Deserialize, DeserializeExtra, OnDeserialized (`0x113cfc8/0x113cfdc/0x113cfec`). Não chama RemoveAllSystems no próprio corpo examinado. Antes de reconstruir o carregamento completo, é necessário seguir o caller **World.Deserialize37014**, que chama este método em `0x1135968`, para verificar preparação do estado.

As funções Serialize/Deserialize de AnimationSystem são vazias (relatório30), mas isso não exclui seus componentes/entidades do save. Como o wrapper não escreve identificadores próprios e despacha métodos pela ordem da coleção, **não reordenar sistemas durante reconstrução do stream** sem recuperar o contrato completo. Isso ainda não é especificação do arquivo de save e nenhum save foi acessado.

## Validação e próximo passo

Doze novos intervalos com cobertura integral; vtables BaseSystem4396, BasePauseSystem4395 e AnimationSystem4450 consultadas na metadata; destinos diretos conferidos por mapa IL2CPP. Saída parcial de xrefs em `ecs-lifecycle/references.json` conserva a limitação a BL/B imediato. Nenhum código C++ alterado e nenhum novo teste de comportamento alegado.

Próximos alvos: resolver interface de PauseSystems e hooks concretos de BasePauseSystem; fechar preparação de World.Deserialize; identificar escritores/assinantes dos delegates; observar quais sistemas filtram enabled dentro do próprio NeedsUpdate. Essas são questões estáticas úteis, não motivo para exigir Android agora.
